#include "SphereCollider.h"
#include "Math/Mathf.h"
#include "Scene/Transform.h"
#include "Serialize/Serializer.h"

using namespace ou;


void ou::SphereCollider::Serialize(Serializer &serializer) {
	Collider::Serialize(serializer);

	serializer.Field("center", center_);
	serializer.Field("radius", radius_);
}


Vector3 ou::SphereCollider::ClosestPoint(const Vector3 &position) const {
	const auto worldCenter = CalculateWorldCenter();
	const auto worldRadius = CalculateWorldRadius();

	const auto offset = position - worldCenter;
	if (offset.LengthSquared() <= worldRadius * worldRadius) {
		return position;
	}
	return worldCenter + offset.Normalized() * worldRadius;
}


Bounds ou::SphereCollider::GetBounds() const {
	return Bounds(CalculateWorldCenter(), Vector3::One * (CalculateWorldRadius() * 2.0f));
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


PhysicsWorld::ShapeGeometry ou::SphereCollider::BuildShapeGeometry() const {
	return {.type = PhysicsWorld::ShapeGeometry::Type::Sphere,
			.radius = radius_};
}


Vector3 ou::SphereCollider::GetShapeCenter() const {
	return center_;
}


Vector3 ou::SphereCollider::CalculateWorldCenter() const {
	return GetTransform()->GetLocalToWorldMatrix().MultiplyPoint3x4(center_);
}


float ou::SphereCollider::CalculateWorldRadius() const {
	const auto scale = GetTransform()->GetLossyScale();
	return radius_ * Mathf::Max(scale.x, scale.y, scale.z);
}
