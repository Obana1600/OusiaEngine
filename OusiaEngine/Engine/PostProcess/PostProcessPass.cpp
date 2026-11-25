#include "PostProcessPass.h"
#include "Asset/AssetDatabase.h"
#include "Diagnostics/Fatal.h"
#include "Editor/SceneRenderTarget.h"
#include "Graphics/GraphicsDevice.h"
#include "Math/Mathf.h"
#include "Math/Vector3.h"
#include "PostProcess/Builtin/ColorAdjustments.h"
#include "PostProcess/Builtin/Tonemapping.h"
#include "PostProcess/VolumeStack.h"
#include "Render/Asset/ShaderModule.h"

#include <cstdint>
#include <memory>

using namespace ou;


namespace {

std::shared_ptr<ShaderModule> sFullScreenVs;
std::shared_ptr<ShaderModule> sUberPostPs;
Graphics::PipelineState sUberPostPso;


struct UberPostConstants {
	uint32_t sourceIndex;
	uint32_t tonemappingMode;  // TonemappingMode（0 ならかけない）
	uint32_t colorAdjustments; // 0 ならかけない（何も変えない値でも、色の変換を往復すると少しずれるため）
	float postExposure;		   // 掛ける数（2 の EV 乗）
	Vector3 colorFilter;	   // 線形の色
	float contrast;			   // 1 で変えない
	float hueShift;			   // 1 周を 1 にしたもの
	float saturation;		   // 1 で変えない
	float padding[2];
};
static_assert(sizeof(UberPostConstants) == 48);

void SetColorAdjustments(const ColorAdjustments &colorAdjustments, UberPostConstants &constants) {
	constants.colorAdjustments = 1;
	constants.postExposure = Mathf::Pow(2.0f, colorAdjustments.postExposure_.GetValue());

	const auto filter = colorAdjustments.colorFilter_.GetValue().Linear();
	constants.colorFilter = {filter.r, filter.g, filter.b};
	constants.contrast = colorAdjustments.contrast_.GetValue() / 100.0f + 1.0f;
	constants.hueShift = colorAdjustments.hueShift_.GetValue() / 360.0f;
	constants.saturation = colorAdjustments.saturation_.GetValue() / 100.0f + 1.0f;
}

} // namespace



void ou::PostProcessPass::StaticInitialize() {
	sFullScreenVs = AssetDatabase::LoadAssetAtPath<ShaderModule>("EngineAssets/Shader/Blit.VS.hlsl");
	sUberPostPs = AssetDatabase::LoadAssetAtPath<ShaderModule>("EngineAssets/Shader/UberPost.PS.hlsl");
	if (!sFullScreenVs || !sUberPostPs) {
		FatalError("UberPost のシェーダの作成に失敗しました");
	}

	Graphics::PipelineStateDesc desc{};
	desc.vs = &sFullScreenVs->GetGraphicsShader();
	desc.ps = &sUberPostPs->GetGraphicsShader();
	if (!Graphics::GetDevice()->CreatePipelineState(&desc, &sUberPostPso)) {
		FatalError("UberPost の PSO の作成に失敗しました");
	}
}


void ou::PostProcessPass::StaticFinalize() {
	sUberPostPso = {};
	sUberPostPs.reset();
	sFullScreenVs.reset();
}


void ou::PostProcessPass::Render(const SceneRenderTarget &target, const VolumeStack *stack, Graphics::CommandList cmd) {
	auto *device = Graphics::GetDevice();

	target.BeginPostProcessPass(cmd);
	device->BindPipelineState(&sUberPostPso, cmd);

	UberPostConstants constants{};
	constants.sourceIndex = static_cast<uint32_t>(device->GetDescriptorIndex(&target.GetSceneColor(), Graphics::ViewType::SRV));
	if (stack) {
		if (const auto *colorAdjustments = stack->GetComponent<ColorAdjustments>(); colorAdjustments && colorAdjustments->IsActive()) {
			SetColorAdjustments(*colorAdjustments, constants);
		}
		if (const auto *tonemapping = stack->GetComponent<Tonemapping>(); tonemapping && tonemapping->IsActive()) {
			constants.tonemappingMode = static_cast<uint32_t>(tonemapping->mode_.GetValue());
		}
	}
	device->BindDynamicConstantBuffer(constants, 0, cmd);

	device->Draw(3, 0, cmd);
	device->RenderPassEnd(cmd);
}
