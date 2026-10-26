#pragma once

#include "Math/Vector3.h"


namespace ou {

// 前方宣言
class Collider;
class Rigidbody;
class Transform;

struct RaycastHit {
	Vector3 point = Vector3::Zero;
	Vector3 normal = Vector3::Zero;
	float distance = 0.0f;
	Collider *collider = nullptr;

	Rigidbody *GetRigidbody() const;
	Transform *GetTransform() const;
};

} // namespace ou
