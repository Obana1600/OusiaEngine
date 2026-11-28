#include "AdditionalLightsShadowCasterPass.h"
#include "Diagnostics/Fatal.h"
#include "Graphics/GraphicsDevice.h"
#include "Math/Mathf.h"
#include "Render/Camera.h"
#include "Render/GraphicsSettings.h"
#include "Render/Light.h"
#include "Render/PerFrame.h"
#include "Render/Shadow/ShadowUtils.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

using namespace ou;


namespace {

Graphics::Texture sAtlas;
uint32_t sAtlasResolution = 0;


struct ShadowSlice {
	const Light *light;
	uint32_t face;
	uint32_t resolution;
	uint32_t offsetX;
	uint32_t offsetY;

	Camera::PerCamera camera;
	Vector4 shadowBias;
};

void CreateAtlas(uint32_t resolution) {
	Graphics::TextureDesc desc{};
	desc.width = resolution;
	desc.height = resolution;
	desc.format = Graphics::Format::D32_FLOAT;
	desc.bindFlags = Graphics::BindFlag::DEPTH_STENCIL | Graphics::BindFlag::SHADER_RESOURCE;
	desc.layout = Graphics::ResourceState::SHADER_RESOURCE;
	desc.clear.depthStencil.depth = 1.0f;

	auto *device = Graphics::GetDevice();
	if (!device->CreateTexture(&desc, nullptr, &sAtlas)) {
		FatalError("Spot / Point の影のアトラスの作成に失敗しました");
	}
	device->SetName(&sAtlas, "AdditionalLightsShadowmap");
	sAtlasResolution = resolution;
}

uint32_t ToPixels(ShadowResolution resolution) {
	return 256u << static_cast<uint32_t>(resolution);
}

uint32_t ToPixels(LightShadowResolution resolution) {
	return 256u << static_cast<uint32_t>(resolution);
}


struct CubeFace {
	Vector3 right;
	Vector3 up;
	Vector3 forward;
};


Camera::PerCamera MakeSpotCamera(const Light *light) {
	const auto *transform = light->GetTransform();
	const auto position = transform->GetWorldPosition();
	const auto rotation = transform->GetWorldRotation();

	Camera::PerCamera camera{};
	camera.view = Matrix4x4::SRT(Vector3::One, rotation, position).Inverse();
	camera.projection = Matrix4x4::Perspective(light->GetSpotAngle() * Mathf::Deg2Rad, 1.0f,
											   light->GetShadowNearPlane(), light->GetRange());
	camera.viewProjection = camera.view * camera.projection;
	camera.worldPosition = position;
	return camera;
}

Camera::PerCamera MakePointCamera(const Light *light, uint32_t face, float fovDegrees) {
	static const CubeFace kCubeFaces[6] = {
		{Vector3::Back, Vector3::Up, Vector3::Right},	   // +X
		{Vector3::Forward, Vector3::Up, Vector3::Left},	   // -X
		{Vector3::Right, Vector3::Back, Vector3::Up},	   // +Y
		{Vector3::Right, Vector3::Forward, Vector3::Down}, // -Y
		{Vector3::Right, Vector3::Up, Vector3::Forward},   // +Z
		{Vector3::Left, Vector3::Up, Vector3::Back},	   // -Z
	};

	const auto position = light->GetTransform()->GetWorldPosition();
	const auto &axes = kCubeFaces[face];

	Camera::PerCamera camera{};
	camera.view = Matrix4x4(axes.right.x, axes.up.x, axes.forward.x, 0.0f,
							axes.right.y, axes.up.y, axes.forward.y, 0.0f,
							axes.right.z, axes.up.z, axes.forward.z, 0.0f,
							-Vector3::Dot(position, axes.right), -Vector3::Dot(position, axes.up), -Vector3::Dot(position, axes.forward), 1.0f);
	camera.projection = Matrix4x4::Perspective(fovDegrees * Mathf::Deg2Rad, 1.0f,
											   light->GetShadowNearPlane(), light->GetRange());
	camera.viewProjection = camera.view * camera.projection;
	camera.worldPosition = position;
	return camera;
}

Vector4 CalculatePunctualShadowBias(const Light *light, float fovDegrees, uint32_t resolution, float softShadowBiasScale) {
	const auto frustumSize = Mathf::Tan(fovDegrees * 0.5f * Mathf::Deg2Rad) * light->GetRange();
	auto biasScale = frustumSize / static_cast<float>(resolution);
	if (light->GetShadows() == LightShadows::Soft) { biasScale *= softShadowBiasScale; }

	// Point は法線方向にずらさない
	const auto normalBias = light->GetType() == LightType::Point ? 0.0f : light->GetShadowNormalBias();

	return Vector4(-light->GetShadowBias() * biasScale,
				   -normalBias * biasScale,
				   1.0f,
				   0.0f);
}

float CalculatePointLightFovBias(uint32_t resolution, bool soft) {
	struct Entry {
		uint32_t maxResolution;
		float hard;
		float softExtra;
	};
	constexpr Entry kTable[] = {
		{16, 43.0f, 9.35f},
		{32, 18.55f, 9.35f},
		{64, 8.63f, 4.07f},
		{128, 4.13f, 1.77f},
		{256, 2.03f, 0.85f},
		{512, 1.00f, 0.39f},
		{1024, 0.50f, 0.17f},
		{2048, 0.25f, 0.074f},
	};

	for (const auto &entry : kTable) {
		if (resolution <= entry.maxResolution) { return entry.hard + (soft ? entry.softExtra : 0.0f); }
	}
	return 4.0f;
}

void DecodeMortonOrder(uint32_t n, uint32_t &x, uint32_t &y) {
	x = 0;
	y = 0;
	for (uint32_t bit = 0; bit < 16; bit++) {
		x |= ((n >> (2 * bit)) & 1u) << bit;
		y |= ((n >> (2 * bit + 1)) & 1u) << bit;
	}
}

void PackSlices(std::vector<ShadowSlice> &slices, uint32_t atlasResolution) {
	const auto atlasArea = static_cast<uint64_t>(atlasResolution) * atlasResolution;

	for (;;) {
		uint64_t totalArea = 0;
		for (const auto &slice : slices) {
			totalArea += static_cast<uint64_t>(slice.resolution) * slice.resolution;
		}
		if (totalArea <= atlasArea) { break; }

		for (auto &slice : slices) {
			slice.resolution /= 2;
		}
	}

	std::vector<ShadowSlice *> order;
	for (auto &slice : slices) {
		order.push_back(&slice);
	}
	std::stable_sort(order.begin(), order.end(), [](const auto *a, const auto *b) {
		return a->resolution > b->resolution;
	});

	uint64_t usedArea = 0;
	for (auto *slice : order) {
		const auto area = static_cast<uint64_t>(slice->resolution) * slice->resolution;
		uint32_t x;
		uint32_t y;
		DecodeMortonOrder(static_cast<uint32_t>(usedArea / area), x, y);
		slice->offsetX = x * slice->resolution;
		slice->offsetY = y * slice->resolution;
		usedArea += area;
	}
}

Matrix4x4 MakeNdcToAtlasUv(const ShadowSlice &slice, uint32_t atlasResolution) {
	const auto atlas = static_cast<float>(atlasResolution);
	const auto scale = static_cast<float>(slice.resolution) / atlas;
	const auto centerX = (0.5f * static_cast<float>(slice.resolution) + static_cast<float>(slice.offsetX)) / atlas;
	const auto centerY = (0.5f * static_cast<float>(slice.resolution) + static_cast<float>(slice.offsetY)) / atlas;

	return Matrix4x4(0.5f * scale, 0.0f, 0.0f, 0.0f,
					 0.0f, -0.5f * scale, 0.0f, 0.0f,
					 0.0f, 0.0f, 1.0f, 0.0f,
					 centerX, centerY, 0.0f, 1.0f);
}

} // namespace



void ou::AdditionalLightsShadowCasterPass::StaticInitialize() {
	CreateAtlas(ToPixels(GraphicsSettings::GetInstance().additionalLightsShadowmapResolution_));
}


void ou::AdditionalLightsShadowCasterPass::StaticFinalize() {
	sAtlas = {};
	sAtlasResolution = 0;
}


void ou::AdditionalLightsShadowCasterPass::Render(const std::vector<const Light *> &lights, PerFrame &perFrame, Graphics::CommandList cmd) {
	auto *device = Graphics::GetDevice();

	// 解像度が変わっていたら作り直す
	const auto &settings = GraphicsSettings::GetInstance();
	if (const auto resolution = ToPixels(settings.additionalLightsShadowmapResolution_); resolution != sAtlasResolution) {
		CreateAtlas(resolution);
	}

	const auto atlas = static_cast<float>(sAtlasResolution);
	perFrame.additionalShadowmapIndex = static_cast<uint32_t>(device->GetDescriptorIndex(&sAtlas, Graphics::ViewType::SRV));
	perFrame.additionalShadowmapSize = Vector4(1.0f / atlas, 1.0f / atlas, atlas, atlas);

	std::vector<ShadowSlice> slices;
	for (size_t i = 0; i < lights.size(); i++) {
		const auto *light = lights[i];
		if (light->GetShadows() == LightShadows::None) { continue; }

		// Spot / Point だけ集める
		const auto type = light->GetType();
		if (type != LightType::Spot && type != LightType::Point) { continue; }

		// Point: 6 面 / Spot: 1 面
		const uint32_t sliceCount = (type == LightType::Point) ? 6u : 1u;
		if (slices.size() + sliceCount > PerFrame::kMaxShadowSlices) { break; }

		auto &shaderLight = perFrame.lights[i];
		shaderLight.shadowStrength = light->GetShadowStrength();
		shaderLight.shadowSoft = (light->GetShadows() == LightShadows::Soft) ? 1u : 0u;
		shaderLight.shadowSliceIndex = static_cast<int32_t>(slices.size());

		const auto resolution = ToPixels(light->GetShadowResolution());
		for (uint32_t face = 0; face < sliceCount; face++) {
			ShadowSlice slice{};
			slice.light = light;
			slice.face = face;
			slice.resolution = resolution;
			slices.push_back(std::move(slice));
		}
	}
	perFrame.additionalShadowFadeParams = ShadowUtils::CalculateShadowFadeParams(settings.shadowDistance_);
	if (slices.empty()) { return; }

	PackSlices(slices, sAtlasResolution);

	for (size_t i = 0; i < slices.size(); i++) {
		auto &slice = slices[i];
		const auto *light = slice.light;

		auto fovDegrees = light->GetSpotAngle();
		if (light->GetType() == LightType::Point) {
			fovDegrees = 90.0f + CalculatePointLightFovBias(slice.resolution, light->GetShadows() == LightShadows::Soft);
			slice.camera = MakePointCamera(slice.light, slice.face, fovDegrees);
		} else {
			slice.camera = MakeSpotCamera(slice.light);
		}
		slice.shadowBias = CalculatePunctualShadowBias(light, fovDegrees, slice.resolution, ShadowUtils::kSoftShadowBiasScale);

		perFrame.additionalLightsWorldToShadow[i] = slice.camera.viewProjection * MakeNdcToAtlasUv(slice, sAtlasResolution);
	}

	// 描画する
	using Image = Graphics::RenderPassImage;
	const auto image = Image::DepthStencil(&sAtlas, Image::LoadOp::CLEAR, Image::StoreOp::STORE,
										   Graphics::ResourceState::SHADER_RESOURCE,
										   Graphics::ResourceState::DEPTHSTENCIL,
										   Graphics::ResourceState::SHADER_RESOURCE);
	device->RenderPassBegin(&image, 1, cmd);

	auto casterFrame = perFrame;
	for (const auto &slice : slices) {
		casterFrame.shadowBias = slice.shadowBias;
		casterFrame.shadowLightPosition = slice.camera.worldPosition;
		device->BindDynamicConstantBuffer(casterFrame, 3, cmd);

		Graphics::Viewport viewport{};
		viewport.topLeftX = static_cast<float>(slice.offsetX);
		viewport.topLeftY = static_cast<float>(slice.offsetY);
		viewport.width = static_cast<float>(slice.resolution);
		viewport.height = static_cast<float>(slice.resolution);
		device->BindViewports(1, &viewport, cmd);

		Graphics::Scissor scissor{};
		scissor.left = static_cast<int32_t>(slice.offsetX);
		scissor.top = static_cast<int32_t>(slice.offsetY);
		scissor.right = static_cast<int32_t>(slice.offsetX + slice.resolution);
		scissor.bottom = static_cast<int32_t>(slice.offsetY + slice.resolution);
		device->BindScissorRects(1, &scissor, cmd);

		SceneManager::RenderShadowCasters(slice.camera, cmd);
	}
	device->RenderPassEnd(cmd);
}
