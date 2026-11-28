#include "MainLightShadowCasterPass.h"
#include "Diagnostics/Fatal.h"
#include "Graphics/GraphicsDevice.h"
#include "Math/Mathf.h"
#include "Render/GraphicsSettings.h"
#include "Render/Light.h"
#include "Render/PerFrame.h"
#include "Render/Shadow/ShadowUtils.h"
#include "Scene/SceneManager.h"
#include "Math/GeometryUtility.h"
#include "Scene/Transform.h"

#include <array>
#include <cstdint>

using namespace ou;


namespace {

const Matrix4x4 kNdcToUv(0.5f, 0.0f, 0.0f, 0.0f,
						 0.0f, -0.5f, 0.0f, 0.0f,
						 0.0f, 0.0f, 1.0f, 0.0f,
						 0.5f, 0.5f, 0.0f, 1.0f);


Graphics::Texture sShadowmap;
uint32_t sResolution = 0;


void CreateShadowmap(uint32_t resolution) {
	Graphics::TextureDesc desc{};
	desc.width = resolution;
	desc.height = resolution;
	desc.format = Graphics::Format::D32_FLOAT;
	desc.bindFlags = Graphics::BindFlag::DEPTH_STENCIL | Graphics::BindFlag::SHADER_RESOURCE;
	desc.layout = Graphics::ResourceState::SHADER_RESOURCE;
	desc.clear.depthStencil.depth = 1.0f;

	auto *device = Graphics::GetDevice();
	if (!device->CreateTexture(&desc, nullptr, &sShadowmap)) {
		FatalError("シャドウマップの作成に失敗しました");
	}
	device->SetName(&sShadowmap, "MainLightShadowmap");
	sResolution = resolution;
}

uint32_t ToPixels(ShadowResolution resolution) {
	return 256u << static_cast<uint32_t>(resolution);
}

float CalculateNdcDepth(const Matrix4x4 &projection, float distance) {
	const auto z = distance * projection.m33 + projection.m43;
	const auto w = distance * projection.m34 + projection.m44;
	return z / w;
}


struct BoundingSphere {
	Vector3 center;
	float radius;
};

BoundingSphere CalculateCameraSphere(const Camera::PerCamera &camera, float shadowDistance) {
	const auto inverse = camera.viewProjection.Inverse();
	const auto farNdc = Mathf::Min(CalculateNdcDepth(camera.projection, shadowDistance), 1.0f);

	std::array<Vector3, 8> corners;
	size_t count = 0;
	for (const auto z : {0.0f, farNdc}) {
		for (const auto y : {-1.0f, 1.0f}) {
			for (const auto x : {-1.0f, 1.0f}) {
				corners[count++] = inverse.MultiplyPoint(Vector3(x, y, z));
			}
		}
	}

	auto center = Vector3::Zero;
	for (const auto &corner : corners) {
		center += corner;
	}
	center /= static_cast<float>(corners.size());

	float radius = 0.0f;
	for (const auto &corner : corners) {
		radius = Mathf::Max(radius, (corner - center).Length());
	}
	radius = Mathf::Ceil(radius * 16.0f) / 16.0f;

	return BoundingSphere(center, radius);
}

} // namespace



void ou::MainLightShadowCasterPass::StaticInitialize() {
	CreateShadowmap(ToPixels(GraphicsSettings::GetInstance().mainLightShadowmapResolution_));
}


void ou::MainLightShadowCasterPass::StaticFinalize() {
	sShadowmap = {};
	sResolution = 0;
}


void ou::MainLightShadowCasterPass::Render(const Camera::PerCamera &camera, PerFrame &perFrame, Graphics::CommandList cmd) {
	auto *device = Graphics::GetDevice();

	const auto *light = SceneManager::FindMainLight();
	if (!light) {
		device->BindDynamicConstantBuffer(perFrame, 3, cmd);
		return;
	}

	// 解像度が前フレームと変わっていたら作り直す
	const auto &settings = GraphicsSettings::GetInstance();
	if (const auto resolution = ToPixels(settings.mainLightShadowmapResolution_); resolution != sResolution) {
		CreateShadowmap(resolution);
	}

	const auto sphere = CalculateCameraSphere(camera, settings.shadowDistance_);
	const auto texelSize = 2.0f * sphere.radius / static_cast<float>(sResolution);

	const auto lightRotation = light->GetTransform()->GetWorldRotation();
	const auto lightToWorld = Matrix4x4::Rotate(lightRotation);
	const auto worldToLight = lightToWorld.Inverse();
	auto centerInLight = worldToLight.MultiplyPoint3x4(sphere.center);
	centerInLight.x = Mathf::Floor(centerInLight.x / texelSize) * texelSize;
	centerInLight.y = Mathf::Floor(centerInLight.y / texelSize) * texelSize;
	const auto center = lightToWorld.MultiplyPoint3x4(centerInLight);

	const auto forward = lightRotation * Vector3::Forward;
	const auto eye = center - forward * sphere.radius;

	Camera::PerCamera lightCamera{};
	lightCamera.view = Matrix4x4::SRT(Vector3::One, lightRotation, eye).Inverse();
	lightCamera.projection = Matrix4x4::Ortho(2.0f * sphere.radius, 2.0f * sphere.radius, 0.0f, 2.0f * sphere.radius);
	lightCamera.viewProjection = lightCamera.view * lightCamera.projection;
	lightCamera.worldPosition = eye;

	// バイアス
	const auto soft = light->GetShadows() == LightShadows::Soft;
	auto biasScale = texelSize;
	if (soft) { biasScale *= ShadowUtils::kSoftShadowBiasScale; }

	// バッファに詰める
	const auto resolution = static_cast<float>(sResolution);
	const auto fade = ShadowUtils::CalculateShadowFadeParams(settings.shadowDistance_);
	perFrame.worldToShadow = lightCamera.viewProjection * kNdcToUv;
	perFrame.shadowBias = {-light->GetShadowBias() * biasScale, -light->GetShadowNormalBias() * biasScale, 0.0f, 0.0f};
	perFrame.shadowLightDirection = -forward;
	perFrame.shadowmapIndex = static_cast<uint32_t>(device->GetDescriptorIndex(&sShadowmap, Graphics::ViewType::SRV));
	perFrame.shadowParams = {light->GetShadowStrength(), soft ? 1.0f : 0.0f, fade.x, fade.y};
	perFrame.shadowmapSize = {1.0f / resolution, 1.0f / resolution, resolution, resolution};
	device->BindDynamicConstantBuffer(perFrame, 3, cmd);

	// 描画する
	using Image = Graphics::RenderPassImage;
	const auto image = Image::DepthStencil(&sShadowmap, Image::LoadOp::CLEAR, Image::StoreOp::STORE,
										   Graphics::ResourceState::SHADER_RESOURCE,
										   Graphics::ResourceState::DEPTHSTENCIL,
										   Graphics::ResourceState::SHADER_RESOURCE);
	device->RenderPassBegin(&image, 1, cmd);

	Graphics::Viewport viewport{};
	viewport.width = static_cast<float>(sResolution);
	viewport.height = static_cast<float>(sResolution);
	device->BindViewports(1, &viewport, cmd);

	Graphics::Scissor scissor{};
	scissor.right = static_cast<int32_t>(sResolution);
	scissor.bottom = static_cast<int32_t>(sResolution);
	device->BindScissorRects(1, &scissor, cmd);

	const auto frustumPlanes = GeometryUtility::CalculateFrustumPlanes(lightCamera.viewProjection);
	const auto cullingPlanes = {frustumPlanes[0], frustumPlanes[1], frustumPlanes[2], frustumPlanes[3], frustumPlanes[5]};
	SceneManager::RenderShadowCasters(lightCamera, cullingPlanes, cmd);

	device->RenderPassEnd(cmd);
}
