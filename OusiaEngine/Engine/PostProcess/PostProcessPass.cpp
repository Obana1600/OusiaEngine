#include "PostProcessPass.h"
#include "Asset/AssetDatabase.h"
#include "Diagnostics/Fatal.h"
#include "Diagnostics/Log.h"
#include "Editor/SceneRenderTarget.h"
#include "Graphics/GraphicsDevice.h"
#include "Math/Color.h"
#include "Math/Mathf.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "PostProcess/Builtin/Bloom.h"
#include "PostProcess/Builtin/ColorAdjustments.h"
#include "PostProcess/Builtin/Tonemapping.h"
#include "PostProcess/Builtin/Vignette.h"
#include "PostProcess/VolumeStack.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Shader.h"
#include "Render/Asset/ShaderModule.h"
#include "Render/GraphicsSettings.h"

#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

using namespace ou;


namespace {

struct FullScreenPass {
	std::shared_ptr<ShaderModule> ps;
	Graphics::PipelineState pso;
};

std::shared_ptr<ShaderModule> sFullScreenVs;
FullScreenPass sUberPost;
FullScreenPass sBloomPrefilter;
FullScreenPass sBloomBlurH;
FullScreenPass sBloomBlurV;
FullScreenPass sBloomUpsample;
FullScreenPass sBlit;


// ----- ポストプロセス -----

struct BloomConstants {
	Vector3 tint;
	uint32_t textureIndex;
	uint32_t enabled;
	uint32_t padding[3];
};
static_assert(sizeof(BloomConstants) % 16 == 0);

struct ColorAdjustmentsConstants {
	float postExposure;
	float contrast;
	float hueShift;
	float saturation;
	Vector3 colorFilter;
	uint32_t enabled;
};
static_assert(sizeof(ColorAdjustmentsConstants) % 16 == 0);

struct VignetteConstants {
	Vector3 color;
	float intensity;
	Vector2 center;
	float smoothness;
	float roundness;
};
static_assert(sizeof(VignetteConstants) % 16 == 0);

struct UberPostConstants {
	uint32_t sourceIndex;
	uint32_t tonemappingMode;
	uint32_t padding[2];
	BloomConstants bloom;
	ColorAdjustmentsConstants colorAdjustments;
	VignetteConstants vignette;
};
static_assert(sizeof(UberPostConstants) == 112);


// ----- Bloom Pass -----

struct BloomPassConstants {
	uint32_t sourceIndex;
	uint32_t lowIndex;
	Vector2 sourceTexelSize;
	float scatter;
	float clampMax;
	float threshold;
	float thresholdKnee;
};
static_assert(sizeof(BloomPassConstants) == 32);


// ----- Full Screen Pass -----

struct BlitConstants {
	uint32_t sourceIndex;
	uint32_t padding[3];
};

struct FullScreenConstants {
	uint32_t blitTextureIndex;
	uint32_t padding[3];
	Vector4 screenSize; // (幅, 高さ, 1 / 幅, 1 / 高さ)
};
static_assert(sizeof(FullScreenConstants) == 32);


void CreateFullScreenPass(const std::string &psPath, FullScreenPass &pass) {
	pass.ps = AssetDatabase::LoadAssetAtPath<ShaderModule>(psPath);
	if (!pass.ps) { FatalError(psPath + " のシェーダの作成に失敗しました"); }

	Graphics::PipelineStateDesc desc{};
	desc.vs = &sFullScreenVs->GetGraphicsShader();
	desc.ps = &pass.ps->GetGraphicsShader();
	if (!Graphics::GetDevice()->CreatePipelineState(&desc, &pass.pso)) {
		FatalError(psPath + " の PSO の作成に失敗しました");
	}
}

uint32_t ToSrvIndex(const Graphics::Texture &texture) {
	return static_cast<uint32_t>(Graphics::GetDevice()->GetDescriptorIndex(&texture, Graphics::ViewType::SRV));
}

Vector2 ToTexelSize(const Graphics::Texture &texture) {
	const auto &desc = texture.GetDesc();
	return {1.0f / static_cast<float>(desc.width), 1.0f / static_cast<float>(desc.height)};
}

void BeginFullScreen(const Graphics::Texture &destination, Graphics::RenderPassImage::LoadOp loadOp, Graphics::CommandList cmd) {
	auto *device = Graphics::GetDevice();

	const auto image = Graphics::RenderPassImage::RenderTarget(&destination, loadOp);
	device->RenderPassBegin(&image, 1, cmd);

	const auto &desc = destination.GetDesc();
	Graphics::Viewport viewport{};
	viewport.width = static_cast<float>(desc.width);
	viewport.height = static_cast<float>(desc.height);
	device->BindViewports(1, &viewport, cmd);

	Graphics::Scissor scissor{};
	scissor.right = static_cast<int32_t>(desc.width);
	scissor.bottom = static_cast<int32_t>(desc.height);
	device->BindScissorRects(1, &scissor, cmd);
}

template <typename T>
void DrawFullScreen(const FullScreenPass &pass, const Graphics::Texture &destination, const T &constants, Graphics::CommandList cmd) {
	auto *device = Graphics::GetDevice();
	BeginFullScreen(destination, Graphics::RenderPassImage::LoadOp::DONTCARE, cmd);
	device->BindPipelineState(&pass.pso, cmd);
	device->BindDynamicConstantBuffer(constants, 0, cmd);
	device->Draw(3, 0, cmd);
	device->RenderPassEnd(cmd);
}

const Graphics::Texture &RenderBloom(const SceneRenderTarget &target, const Bloom &bloom, Graphics::CommandList cmd) {
	BloomPassConstants constants{};

	const auto threshold = bloom.threshold_.GetValue();
	constants.threshold = Color(threshold, threshold, threshold, 1.0f).Linear().r;
	constants.thresholdKnee = constants.threshold * 0.5f;
	constants.scatter = Mathf::Lerp(0.05f, 0.95f, bloom.scatter_.GetValue());
	constants.clampMax = bloom.clamp_.GetValue();

	constants.sourceIndex = ToSrvIndex(target.GetSceneColor());
	constants.sourceTexelSize = ToTexelSize(target.GetSceneColor());
	DrawFullScreen(sBloomPrefilter, target.GetBloomMipDown(0), constants, cmd);

	const auto mipCount = target.GetBloomMipCount();
	for (uint32_t i = 1; i < mipCount; i++) {
		const auto &larger = target.GetBloomMipDown(i - 1);
		constants.sourceIndex = ToSrvIndex(larger);
		constants.sourceTexelSize = ToTexelSize(larger);
		DrawFullScreen(sBloomBlurH, target.GetBloomMipUp(i), constants, cmd);

		const auto &horizontal = target.GetBloomMipUp(i);
		constants.sourceIndex = ToSrvIndex(horizontal);
		constants.sourceTexelSize = ToTexelSize(horizontal);
		DrawFullScreen(sBloomBlurV, target.GetBloomMipDown(i), constants, cmd);
	}
	if (mipCount == 1) { return target.GetBloomMipDown(0); }

	for (auto i = static_cast<int>(mipCount) - 2; i >= 0; i--) {
		const auto index = static_cast<uint32_t>(i);
		const auto &low = index == mipCount - 2 ? target.GetBloomMipDown(index + 1) : target.GetBloomMipUp(index + 1);
		const auto &high = target.GetBloomMipDown(index);
		constants.sourceIndex = ToSrvIndex(high);
		constants.lowIndex = ToSrvIndex(low);
		constants.sourceTexelSize = ToTexelSize(high);
		DrawFullScreen(sBloomUpsample, target.GetBloomMipUp(index), constants, cmd);
	}
	return target.GetBloomMipUp(0);
}


BloomConstants MakeBloomConstants(const Bloom &bloom, const Graphics::Texture &texture) {
	auto tint = bloom.tint_.GetValue().Linear();
	const auto luminance = tint.r * 0.2126729f + tint.g * 0.7151522f + tint.b * 0.0721750f;
	const auto scale = luminance > 0.0f ? bloom.intensity_.GetValue() / luminance : bloom.intensity_.GetValue();
	if (luminance <= 0.0f) { tint = Color::White; }

	BloomConstants constants{};
	constants.tint = {tint.r * scale, tint.g * scale, tint.b * scale};
	constants.textureIndex = ToSrvIndex(texture);
	constants.enabled = 1;
	return constants;
}

ColorAdjustmentsConstants MakeColorAdjustmentsConstants(const ColorAdjustments &colorAdjustments) {
	ColorAdjustmentsConstants constants{};
	constants.postExposure = Mathf::Pow(2.0f, colorAdjustments.postExposure_.GetValue());
	constants.contrast = colorAdjustments.contrast_.GetValue() / 100.0f + 1.0f;
	constants.hueShift = colorAdjustments.hueShift_.GetValue() / 360.0f;
	constants.saturation = colorAdjustments.saturation_.GetValue() / 100.0f + 1.0f;

	const auto filter = colorAdjustments.colorFilter_.GetValue().Linear();
	constants.colorFilter = {filter.r, filter.g, filter.b};
	constants.enabled = 1;
	return constants;
}

VignetteConstants MakeVignetteConstants(const Vignette &vignette, float aspectRatio) {
	VignetteConstants constants{};
	const auto &color = vignette.color_.GetValue();
	constants.color = {color.r, color.g, color.b};
	constants.intensity = vignette.intensity_.GetValue() * 3.0f;
	constants.center = vignette.center_.GetValue();
	constants.smoothness = vignette.smoothness_.GetValue() * 5.0f;
	constants.roundness = vignette.rounded_.GetValue() ? aspectRatio : 1.0f;
	return constants;
}


void WarnOnce(const std::string &message) {
	static std::unordered_set<std::string> sWarned;
	if (sWarned.insert(message).second) { Log::Warning("{}", message); }
}

const Graphics::PipelineState *FindPipelineState(const FullScreenPassRendererFeature &feature) {
	const auto &material = feature.passMaterial_;
	if (!material) {
		WarnOnce(std::format("Full Screen Pass \"{}\" は、Pass Material がないので描けません", feature.GetName()));
		return nullptr;
	}

	const auto &shader = material->GetShader();
	if (!shader || shader->GetPassCount() == 0) {
		WarnOnce(std::format("Full Screen Pass \"{}\" は、マテリアル \"{}\" にシェーダがないので描けません", feature.GetName(), material->GetName()));
		return nullptr;
	}

	return shader->GetPipelineState(0, material->GetRenderStateOverride(), nullptr, Graphics::PrimitiveTopology::TRIANGLELIST);
}

void RenderFullScreenPass(const FullScreenPassRendererFeature &feature, const Graphics::PipelineState &pso, const Graphics::Texture &color, const Graphics::Texture &copy, Graphics::CommandList cmd) {
	auto *device = Graphics::GetDevice();

	if (feature.fetchColorBuffer_) {
		BlitConstants blitConstants{};
		blitConstants.sourceIndex = ToSrvIndex(color);
		DrawFullScreen(sBlit, copy, blitConstants, cmd);
	}

	BeginFullScreen(color, Graphics::RenderPassImage::LoadOp::LOAD, cmd);
	device->BindPipelineState(&pso, cmd);

	const auto &desc = color.GetDesc();
	const auto width = static_cast<float>(desc.width);
	const auto height = static_cast<float>(desc.height);
	FullScreenConstants constants{};
	constants.blitTextureIndex = ToSrvIndex(copy);
	constants.screenSize = {width, height, 1.0f / width, 1.0f / height};
	device->BindDynamicConstantBuffer(constants, 0, cmd);

	const auto &material = *feature.passMaterial_;
	if (const auto size = material.GetShader()->GetPropertyBufferSize(); size > 0) {
		const auto properties = device->AllocateGpu(size, cmd);
		material.WritePropertyBuffer(properties.data);
		device->BindConstantBuffer(&properties.buffer, 1, cmd, properties.offset);
	}

	device->Draw(3, 0, cmd);
	device->RenderPassEnd(cmd);
}

void RenderFullScreenPasses(const std::vector<FullScreenPassRendererFeature> &features, FullScreenPassRendererFeature::InjectionPoint injectionPoint,
							const Graphics::Texture &color, const Graphics::Texture &copy, Graphics::CommandList cmd) {
	for (const auto &feature : features) {
		if (!feature.GetIsActive() || feature.injectionPoint_ != injectionPoint) { continue; }
		if (const auto *pso = FindPipelineState(feature)) {
			RenderFullScreenPass(feature, *pso, color, copy, cmd);
		}
	}
}

} // namespace



void ou::PostProcessPass::StaticInitialize() {
	sFullScreenVs = AssetDatabase::LoadAssetAtPath<ShaderModule>("EngineAssets/Shader/Blit.VS.hlsl");
	if (!sFullScreenVs) { FatalError("Blit.VS.hlsl のシェーダの作成に失敗しました"); }

	CreateFullScreenPass("EngineAssets/Shader/UberPost.PS.hlsl", sUberPost);
	CreateFullScreenPass("EngineAssets/Shader/BloomPrefilter.PS.hlsl", sBloomPrefilter);
	CreateFullScreenPass("EngineAssets/Shader/BloomBlurH.PS.hlsl", sBloomBlurH);
	CreateFullScreenPass("EngineAssets/Shader/BloomBlurV.PS.hlsl", sBloomBlurV);
	CreateFullScreenPass("EngineAssets/Shader/BloomUpsample.PS.hlsl", sBloomUpsample);
	CreateFullScreenPass("EngineAssets/Shader/Blit.PS.hlsl", sBlit);
}


void ou::PostProcessPass::StaticFinalize() {
	for (auto *pass : {&sUberPost, &sBloomPrefilter, &sBloomBlurH, &sBloomBlurV, &sBloomUpsample, &sBlit}) {
		*pass = {};
	}
	sFullScreenVs.reset();
}


void ou::PostProcessPass::Render(const SceneRenderTarget &target, const VolumeStack *stack, Graphics::CommandList cmd) {
	auto *device = Graphics::GetDevice();

	using InjectionPoint = FullScreenPassRendererFeature::InjectionPoint;
	const auto &features = GraphicsSettings::GetInstance().rendererFeatures_;

	RenderFullScreenPasses(features, InjectionPoint::BeforeRenderingPostProcessing, target.GetSceneColor(), target.GetSceneColorCopy(), cmd);

	UberPostConstants constants{};
	constants.sourceIndex = ToSrvIndex(target.GetSceneColor());
	if (stack) {
		if (const auto *bloom = stack->GetComponent<Bloom>(); bloom && bloom->IsActive()) {
			const auto &bloomTexture = RenderBloom(target, *bloom, cmd);
			constants.bloom = MakeBloomConstants(*bloom, bloomTexture);
		}
		if (const auto *colorAdjustments = stack->GetComponent<ColorAdjustments>(); colorAdjustments && colorAdjustments->IsActive()) {
			constants.colorAdjustments = MakeColorAdjustmentsConstants(*colorAdjustments);
		}
		if (const auto *tonemapping = stack->GetComponent<Tonemapping>(); tonemapping && tonemapping->IsActive()) {
			constants.tonemappingMode = static_cast<uint32_t>(tonemapping->mode_.GetValue());
		}
		if (const auto *vignette = stack->GetComponent<Vignette>(); vignette && vignette->IsActive()) {
			const auto aspectRatio = static_cast<float>(target.GetWidth()) / static_cast<float>(target.GetHeight());
			constants.vignette = MakeVignetteConstants(*vignette, aspectRatio);
		}
	}

	target.BeginPostProcessPass(cmd);
	device->BindPipelineState(&sUberPost.pso, cmd);
	device->BindDynamicConstantBuffer(constants, 0, cmd);
	device->Draw(3, 0, cmd);
	device->RenderPassEnd(cmd);

	RenderFullScreenPasses(features, InjectionPoint::AfterRenderingPostProcessing, target.GetColor(), target.GetColorCopy(), cmd);
}
