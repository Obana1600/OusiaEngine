#pragma once

#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Physics/Jolt/PhysicsWorld.h"
#include "Scene/Component.h"


namespace ou {

class Rigidbody final: public Component {
	friend class Physics;
	friend class Collider;

public: // メンバ関数

	void Awake() override;
	void OnDestroy() override;

	void SetMass(float mass);
	float GetMass() const;

	void SetUseGravity(bool useGravity);
	bool GetUseGravity() const;


private: // メンバ変数

	float mass_ = 1.0f;
	bool useGravity_ = true;

	Vector3 position_ = Vector3::Zero;
	Quaternion rotation_ = Quaternion::Identity;

	PhysicsWorld::BodyId bodyId_ = PhysicsWorld::kInvalidBodyId;


private: // メンバ関数

	void SyncBodyPose();
	void WriteBackPose();
};

} // namespace ou
