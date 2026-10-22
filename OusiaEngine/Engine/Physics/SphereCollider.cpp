#include "SphereCollider.h"
#include "Math/Mathf.h"
#include "Scene/Transform.h"

using namespace ou;


Vector3 ou::SphereCollider::ClosestPoint(const Vector3 &position) const {
	const auto worldCenter = CalculateWorldCenter();
	const auto worldRadius = CalculateWorldRadius();

	const auto offset = position - worldCenter;
	if (offset.LengthSquared() <= worldRadius * worldRadius) {
		return position;
	}
	return worldCenter + offset.Normalize() * worldRadius;
}


Bounds ou::SphereCollider::GetBounds() const {
	return Bounds(CalculateWorldCenter(), Vector3::One * (CalculateWorldRadius() * 2.0f));
}


ColliderShape ou::SphereCollider::GetShape() const {
	return ColliderShape::Sphere;
}


void ou::SphereCollider::SetCenter(const Vector3 &center) {
	center_ = center;
}


const Vector3 &ou::SphereCollider::GetCenter() const {
	return center_;
}


void ou::SphereCollider::SetRadius(float radius) {
	radius_ = radius;
}


float ou::SphereCollider::GetRadius() const {
	return radius_;
}


Vector3 ou::SphereCollider::CalculateWorldCenter() const {
	return GetTransform()->GetLocalToWorldMatrix().MultiplyPoint3x4(center_);
}


float ou::SphereCollider::CalculateWorldRadius() const {
	const auto scale = GetTransform()->GetLossyScale();
	return radius_ * Mathf::Max(scale.x, scale.y, scale.z);
}
