#pragma once

#include "Math/Vector3.h"

#include <vector>


namespace ou {

// 前方宣言
class Collider;
class GameObject;
class Rigidbody;
class Transform;

struct ContactPoint {
	Vector3 point = Vector3::Zero;
	Vector3 normal = Vector3::Zero;
	float separation = 0.0f;
	Collider *thisCollider = nullptr;
	Collider *otherCollider = nullptr;
};

class Collision final {
public: // メンバ関数

	Collision(Collider *collider, Rigidbody *rigidbody, std::vector<ContactPoint> contacts);

	Collider *GetCollider() const;
	Rigidbody *GetRigidbody() const;
	GameObject *GetGameObject() const;
	Transform *GetTransform() const;

	int GetContactCount() const;
	const ContactPoint &GetContactPoint(int index) const;
	const std::vector<ContactPoint> &GetContacts() const;


private: // メンバ変数

	Collider *collider_ = nullptr;
	Rigidbody *rigidbody_ = nullptr;
	std::vector<ContactPoint> contacts_;
};

} // namespace ou
