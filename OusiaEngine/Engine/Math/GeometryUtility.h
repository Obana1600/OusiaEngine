#pragma once

#include "Math/Bounds.h"
#include "Math/Plane.h"
#include "Math/Vector3.h"

#include <array>
#include <span>


namespace ou {

// 前方宣言
class Camera;
struct Matrix4x4;


class GeometryUtility final {
public: // メンバ関数

	GeometryUtility() = delete;

	static Bounds CalculateBounds(std::span<const Vector3> positions, const Matrix4x4 &transform);

	static std::array<Plane, 6> CalculateFrustumPlanes(const Camera &camera);
	static std::array<Plane, 6> CalculateFrustumPlanes(const Matrix4x4 &worldToProjectionMatrix);
	static void CalculateFrustumPlanes(const Camera &camera, std::array<Plane, 6> &planes);
	static void CalculateFrustumPlanes(const Matrix4x4 &worldToProjectionMatrix, std::array<Plane, 6> &planes);

	static bool TestPlanesAABB(std::span<const Plane> planes, const Bounds &bounds);
};

} // namespace ou