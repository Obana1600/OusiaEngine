#pragma once

#include "Math/Matrix4x4.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Render/Light.h"

#include <cstdint>


namespace ou {

struct PerFrame {
	static constexpr uint32_t kNoShadowLight = 0xFFFFFFFF;
	static constexpr uint32_t kMaxShadowSlices = 32;

	Vector4 time;

	// ライト
	uint32_t lightCount;
	uint32_t padding[3];
	Light::ShaderLight lights[Light::kMaxLights];

	// Directional の影
	Matrix4x4 worldToShadow;
	Vector4 shadowBias; // x: depth, y: normal
	Vector3 shadowLightDirection;
	uint32_t shadowmapIndex;
	Vector4 shadowParams;						// x: 濃さ, y: Soft なら 1, z / w: 距離で薄くする scale / bias
	Vector4 shadowmapSize;						// (1 / 幅, 1 / 高さ, 幅, 高さ)
	uint32_t shadowLightIndex = kNoShadowLight; // lights の何番目が影を持つか

	// Spot / Point の影
	Vector3 shadowLightPosition;
	Matrix4x4 additionalLightsWorldToShadow[kMaxShadowSlices]; // 区画ごと
	Vector4 additionalShadowmapSize;						   // (1 / 幅, 1 / 高さ, 幅, 高さ)
	uint32_t additionalShadowmapIndex;
	Vector2 additionalShadowFadeParams; // 距離で薄くする scale / bias
	uint32_t additionalShadowPadding;
};

} // namespace ou
