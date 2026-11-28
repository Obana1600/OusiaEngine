#include "GeometryUtility.h"
#include "Math/Matrix4x4.h"
#include "Render/Camera.h"
#include "Math/Mathf.h"

using namespace ou;


namespace {

Plane MakePlane(float a, float b, float c, float d) {
	const auto length = Vector3(a, b, c).Length();
	return Plane(Vector3(a, b, c) / length, d / length);
}

} // namespace



Bounds ou::GeometryUtility::CalculateBounds(std::span<const Vector3> positions, const Matrix4x4 &transform) {
	if (positions.empty()) { return Bounds(); }

	Bounds bounds(transform.MultiplyPoint3x4(positions[0]), Vector3::Zero);
	for (const auto &position : positions.subspan(1)) {
		bounds.Encapsulate(transform.MultiplyPoint3x4(position));
	}
	return bounds;
}


std::array<Plane, 6> ou::GeometryUtility::CalculateFrustumPlanes(const Camera &camera) {
	std::array<Plane, 6> planes;
	CalculateFrustumPlanes(camera, planes);
	return planes;
}


std::array<Plane, 6> ou::GeometryUtility::CalculateFrustumPlanes(const Matrix4x4 &worldToProjectionMatrix) {
	std::array<Plane, 6> planes;
	CalculateFrustumPlanes(worldToProjectionMatrix, planes);
	return planes;
}


void ou::GeometryUtility::CalculateFrustumPlanes(const Camera &camera, std::array<Plane, 6> &planes) {
	const auto worldToProjectionMatrix = camera.GetWorldToCameraMatrix() * camera.GetProjectionMatrix();
	CalculateFrustumPlanes(worldToProjectionMatrix, planes);
}


void ou::GeometryUtility::CalculateFrustumPlanes(const Matrix4x4 &worldToProjectionMatrix, std::array<Plane, 6> &planes) {
	const auto &m = worldToProjectionMatrix;

	planes[0] = MakePlane(m.m14 + m.m11, m.m24 + m.m21, m.m34 + m.m31, m.m44 + m.m41); // left
	planes[1] = MakePlane(m.m14 - m.m11, m.m24 - m.m21, m.m34 - m.m31, m.m44 - m.m41); // right
	planes[2] = MakePlane(m.m14 + m.m12, m.m24 + m.m22, m.m34 + m.m32, m.m44 + m.m42); // down
	planes[3] = MakePlane(m.m14 - m.m12, m.m24 - m.m22, m.m34 - m.m32, m.m44 - m.m42); // up
	planes[4] = MakePlane(m.m13, m.m23, m.m33, m.m43);								   // near
	planes[5] = MakePlane(m.m14 - m.m13, m.m24 - m.m23, m.m34 - m.m33, m.m44 - m.m43); // far
}


bool ou::GeometryUtility::TestPlanesAABB(std::span<const Plane> planes, const Bounds &bounds) {
	for (const auto &plane : planes) {
		const auto radius = Mathf::Abs(plane.normal.x) * bounds.extents.x +
							Mathf::Abs(plane.normal.y) * bounds.extents.y +
							Mathf::Abs(plane.normal.z) * bounds.extents.z;
		if (plane.GetDistanceToPoint(bounds.center) + radius < 0.0f) { return false; }
	}
	return true;
}
