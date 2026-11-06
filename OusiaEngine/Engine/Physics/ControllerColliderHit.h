#pragma once

#include "Math/Vector3.h"


namespace ou {

// 前方宣言
class CharacterController;
class Collider;
class GameObject;
class Rigidbody;
class Transform;

class ControllerColliderHit final {
public: // メンバ関数

	ControllerColliderHit(CharacterController *controller, Collider *collider,
						  const Vector3 &point, const Vector3 &normal,
						  const Vector3 &moveDirection, float moveLength);

	CharacterController *GetController() const;
	Collider *GetCollider() const;
	Rigidbody *GetRigidbody() const;
	GameObject *GetGameObject() const;
	Transform *GetTransform() const;

	const Vector3 &GetPoint() const;
	const Vector3 &GetNormal() const;
	const Vector3 &GetMoveDirection() const;
	float GetMoveLength() const;


private: // メンバ変数

	CharacterController *controller_ = nullptr;
	Collider *collider_ = nullptr;
	Vector3 point_ = Vector3::Zero;
	Vector3 normal_ = Vector3::Zero;
	Vector3 moveDirection_ = Vector3::Zero;
	float moveLength_ = 0.0f;
};

} // namespace ou
