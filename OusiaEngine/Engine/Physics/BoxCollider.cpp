#include "BoxCollider.h"
#include "Math/Mathf.h"
#include "Math/Matrix4x4.h"
#include "Scene/Transform.h"
#include "Serialize/Serializer.h"

using namespace ou;


void ou::BoxCollider::Serialize(Serializer &serializer) {
	Collider::Serialize(serializer);

	serializer.Field("center", center_);
	serializer.Field("size", size_);
}


Vector3 ou::BoxCollider::ClosestPoint(const Vector3 &position) const {
	const auto box = CalculateWorldBox();
	const auto offset = position - box.center;

	auto result = box.center;
	for (int i = 0; i < 3; i++) {
		const auto distance = Mathf::Clamp(
			Vector3::Dot(offset, box.axes[i]), -box.halfExtents[i], box.halfExtents[i]);
		result += box.axes[i] * distance;
	}

	return result;
}


Bounds ou::BoxCollider::GetBounds() const {
	const auto box = CalculateWorldBox();

	Bounds bounds{};
	bounds.center = box.center;
	for (int i = 0; i < 3; i++) {
		bounds.extents[i] = Mathf::Abs(box.axes[0][i]) * box.halfExtents[0] +
							Mathf::Abs(box.axes[1][i]) * box.halfExtents[1] +
							Mathf::Abs(box.axes[2][i]) * box.halfExtents[2];
	}

	return bounds;
}


void ou::BoxCollider::SetCenter(const Vector3 &center) {
	center_ = center;
}


const Vector3 &ou::BoxCollider::GetCenter() const {
	return center_;
}


void ou::BoxCollider::SetSize(const Vector3 &size) {
	size_ = size;
}


const Vector3 &ou::BoxCollider::GetSize() const {
	return size_;
}


PhysicsWorld::ShapeGeometry ou::BoxCollider::BuildShapeGeometry() const {
	return {.type = PhysicsWorld::ShapeGeometry::Type::Box,
			.halfExtents = size_ * 0.5f};
}


Vector3 ou::BoxCollider::GetShapeCenter() const {
	return center_;
}


BoxCollider::WorldBox ou::BoxCollider::CalculateWorldBox() const {
	const auto &m = GetTransform()->GetLocalToWorldMatrix();
	const Vector3 rows[3] = {
		{m.m11, m.m12, m.m13},
		{m.m21, m.m22, m.m23},
		{m.m31, m.m32, m.m33},
	};

	WorldBox box{};
	box.center = m.MultiplyPoint3x4(center_);
	for (int i = 0; i < 3; i++) {
		const auto scale = rows[i].Length();
		box.axes[i] = scale > 0.0f ? rows[i] / scale : Vector3::One;
		box.halfExtents[i] = size_[i] * 0.5f * scale;
	}

	return box;
}
