#include "Plane.h"

#include <format>

using namespace ou;


ou::Plane::Plane()
	: normal(Vector3::Zero),
	  distance(0.0f) {}


ou::Plane::Plane(const Vector3 &inNormal, const Vector3 &inPoint)
	: normal(inNormal.Normalized()),
	  distance(-Vector3::Dot(normal, inPoint)) {}


ou::Plane::Plane(const Vector3 &inNormal, float d)
	: normal(inNormal.Normalized()),
	  distance(d) {}


ou::Plane::Plane(const Vector3 &a, const Vector3 &b, const Vector3 &c) {
	const auto ba = b - a;
	const auto ca = c - a;
	const auto cross = Vector3::Cross(ba, ca);
	normal = cross.Normalized();

	distance = -Vector3::Dot(normal, a);
}


Vector3 ou::Plane::ClosestPointOnPlane(const Vector3 &point) const {
	const auto pointToPlaneDistance = Vector3::Dot(normal, point) + distance;
	return point - (normal * pointToPlaneDistance);
}


float ou::Plane::GetDistanceToPoint(const Vector3 &point) const {
	return Vector3::Dot(normal, point) + distance;
}


bool ou::Plane::GetSide(const Vector3 &point) const {
	return GetDistanceToPoint(point) > 0.0f;
}


bool ou::Plane::SameSide(const Vector3 &inPt0, const Vector3 &inPt1) const {
	const auto d0 = GetDistanceToPoint(inPt0);
	const auto d1 = GetDistanceToPoint(inPt1);
	return (d0 > 0.0f && d1 > 0.0f) ||
		   (d0 <= 0.0f && d1 <= 0.0f);
}


bool ou::Plane::Equals(const Plane &other) const {
	return normal.Equals(other.normal) && (distance == other.distance);
}


std::string ou::Plane::ToString() const {
	return std::format("Normal: {}, Distance: ({:.2f})", normal.ToString(".2f"), distance);
}


std::string ou::Plane::ToString(std::string_view format) const {
	const auto fmt = std::format("Normal: ({{:{0}}}, {{:{0}}}, {{:{0}}}), Distance: ({{:{0}}})", format);
	return std::vformat(fmt, std::make_format_args(normal.x, normal.y, normal.z, distance));
}
