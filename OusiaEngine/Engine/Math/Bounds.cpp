#include "Bounds.h"

#include <format>

using namespace ou;


ou::Bounds::Bounds()
	: center({}),
	  extents({}) {}


ou::Bounds::Bounds(const Vector3 &center, const Vector3 &size)
	: center(center),
	  extents(size * 0.5f) {}


void ou::Bounds::SetMinMax(const Vector3 &min, const Vector3 &max) {
	extents = (max - min) * 0.5f;
	center = min + extents;
}


void ou::Bounds::SetMin(const Vector3 &min) {
	SetMinMax(min, GetMax());
}


void ou::Bounds::SetMax(const Vector3 &max) {
	SetMinMax(GetMin(), max);
}


Vector3 ou::Bounds::GetMin() const {
	return center - extents;
}


Vector3 ou::Bounds::GetMax() const {
	return center + extents;
}


void ou::Bounds::SetSize(const Vector3 &size) {
	extents = size * 0.5f;
}


Vector3 ou::Bounds::GetSize() const {
	return extents * 2.0f;
}


bool ou::Bounds::Contains(const Vector3 &point) const {
	const auto min = GetMin();
	const auto max = GetMax();
	return min.x <= point.x && point.x <= max.x &&
		   min.y <= point.y && point.y <= max.y &&
		   min.z <= point.z && point.z <= max.z;
}


bool ou::Bounds::Intersects(const Bounds &bounds) const {
	const auto aMin = GetMin();
	const auto aMax = GetMax();
	const auto bMin = bounds.GetMin();
	const auto bMax = bounds.GetMax();
	return aMin.x <= bMax.x && bMin.x <= aMax.x &&
		   aMin.y <= bMax.y && bMin.y <= aMax.y &&
		   aMin.z <= bMax.z && bMin.z <= aMax.z;
}


Vector3 ou::Bounds::ClosestPoint(const Vector3 &point) const {
	return Vector3::Max(GetMin(), Vector3::Min(point, GetMax()));
}


float ou::Bounds::SqrDistance(const Vector3 &point) const {
	return (ClosestPoint(point) - point).LengthSquared();
}


void ou::Bounds::Encapsulate(const Vector3 &point) {
	SetMinMax(Vector3::Min(GetMin(), point), Vector3::Max(GetMax(), point));
}


void ou::Bounds::Encapsulate(const Bounds &bounds) {
	Encapsulate(bounds.GetMin());
	Encapsulate(bounds.GetMax());
}


void ou::Bounds::Expand(float amount) {
	const auto half = amount * 0.5f;
	extents += Vector3(half, half, half);
}


std::string ou::Bounds::ToString() const {
	return std::format("Center: ({:.2f}, {:.2f}, {:.2f}), Extents: ({:.2f}, {:.2f}, {:.2f})",
					   center.x, center.y, center.z, extents.x, extents.y, extents.z);
}


std::string ou::Bounds::ToString(std::string_view format) const {
	const auto fmt = std::format("Center: ({{:{0}}}, {{:{0}}}, {{:{0}}}), Extents: ({{:{0}}}, {{:{0}}}, {{:{0}}})", format);
	return std::vformat(fmt, std::make_format_args(center.x, center.y, center.z, extents.x, extents.y, extents.z));
}
