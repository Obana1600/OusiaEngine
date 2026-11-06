#include "CapsuleCollider.h"
#include "Math/Mathf.h"
#include "Math/Matrix4x4.h"
#include "Scene/Transform.h"

#include <algorithm>

using namespace ou;


Vector3 ou::CapsuleCollider::ClosestPoint(const Vector3 &position) const {
	const auto capsule = CalculateWorldCapsule();

	const auto segment = capsule.point1 - capsule.point0;
	const auto lengthSquared = segment.LengthSquared();
	const auto t = lengthSquared > 0.0f ? Mathf::Clamp01(Vector3::Dot(position - capsule.point0, segment) / lengthSquared)
										: 0.0f;
	const auto nearest = capsule.point0 + segment * t;

	const auto offset = position - nearest;
	if (offset.LengthSquared() <= capsule.radius * capsule.radius) {
		return position;
	}

	return nearest + offset.Normalized() * capsule.radius;
}


Bounds ou::CapsuleCollider::GetBounds() const {
	const auto capsule = CalculateWorldCapsule();
	const auto extents = Vector3::One * capsule.radius;

	Bounds bounds;
	bounds.SetMinMax(Vector3::Min(capsule.point0, capsule.point1) - extents,
					 Vector3::Max(capsule.point0, capsule.point1) + extents);

	return bounds;
}


void ou::CapsuleCollider::SetCenter(const Vector3 &center) {
	center_ = center;
}


const Vector3 &ou::CapsuleCollider::GetCenter() const {
	return center_;
}


void ou::CapsuleCollider::SetRadius(float radius) {
	radius_ = Mathf::Max(radius, 0.0f);
}


float ou::CapsuleCollider::GetRadius() const {
	return radius_;
}


void ou::CapsuleCollider::SetHeight(float height) {
	height_ = Mathf::Max(height, 0.0f);
}


float ou::CapsuleCollider::GetHeight() const {
	return height_;
}


void ou::CapsuleCollider::SetDirection(int direction) {
	direction_ = std::clamp(direction, 0, 2);
}


int ou::CapsuleCollider::GetDirection() const {
	return direction_;
}


PhysicsWorld::ShapeGeometry ou::CapsuleCollider::BuildShapeGeometry() const {
	return {.type = PhysicsWorld::ShapeGeometry::Type::Capsule,
			.radius = radius_,
			.height = height_,
			.direction = direction_};
}


Vector3 ou::CapsuleCollider::GetShapeCenter() const {
	return center_;
}


CapsuleCollider::WorldCapsule ou::CapsuleCollider::CalculateWorldCapsule() const {
	const auto *transform = GetTransform();
	const auto scale = transform->GetLossyScale();

	const auto axisScale = Mathf::Abs(scale[direction_]);
	const auto radiusScale = Mathf::Max(Mathf::Abs(scale[(direction_ + 1) % 3]),
										Mathf::Abs(scale[(direction_ + 2) % 3]));

	WorldCapsule capsule{};
	capsule.radius = Mathf::Abs(radius_) * radiusScale;
	const auto halfLength = Mathf::Max(Mathf::Abs(height_) * axisScale * 0.5f - capsule.radius,
									   0.0f);

	auto axis = Vector3::Zero;
	axis[direction_] = 1.0f;
	const auto center = transform->GetLocalToWorldMatrix().MultiplyPoint3x4(center_);
	const auto offset = (transform->GetWorldRotation() * axis) * halfLength;
	capsule.point0 = center - offset;
	capsule.point1 = center + offset;
	return capsule;
}
