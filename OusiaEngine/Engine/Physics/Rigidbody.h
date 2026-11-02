#pragma once

#include "Core/EnumFlags.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Physics/Box3D/PhysicsWorld.h"
#include "Scene/Component.h"

#include <cstdint>


namespace ou {

enum class ForceMode {
	Force,
	Acceleration,
	Impulse,
	VelocityChange,
};

enum class RigidbodyConstraints : uint8_t {
	None = 0,
	FreezePositionX = 1 << 1,
	FreezePositionY = 1 << 2,
	FreezePositionZ = 1 << 3,
	FreezeRotationX = 1 << 4,
	FreezeRotationY = 1 << 5,
	FreezeRotationZ = 1 << 6,
	FreezePosition = FreezePositionX | FreezePositionY | FreezePositionZ,
	FreezeRotation = FreezeRotationX | FreezeRotationY | FreezeRotationZ,
	FreezeAll = FreezePosition | FreezeRotation,
};

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

	void SetIsKinematic(bool isKinematic);
	bool IsKinematic() const;

	void SetLinearDamping(float damping);
	float GetLinearDamping() const;

	void SetAngularDamping(float damping);
	float GetAngularDamping() const;

	void SetConstraints(RigidbodyConstraints constraints);
	RigidbodyConstraints GetConstraints() const;

	void SetLinearVelocity(const Vector3 &velocity);
	Vector3 GetLinearVelocity() const;

	void SetAngularVelocity(const Vector3 &velocity);
	Vector3 GetAngularVelocity() const;

	void AddForce(const Vector3 &force, ForceMode mode = ForceMode::Force);
	void AddTorque(const Vector3 &torque, ForceMode mode = ForceMode::Force);

	void MovePosition(const Vector3 &position);
	void MoveRotation(const Quaternion &rotation);


private: // メンバ変数

	float mass_ = 1.0f;
	bool useGravity_ = true;
	bool isKinematic_ = false;
	float linearDamping_ = 0.0f;
	float angularDamping_ = 0.05f;
	RigidbodyConstraints constraints_ = RigidbodyConstraints::None;

	Vector3 position_ = Vector3::Zero;
	Quaternion rotation_ = Quaternion::Identity;

	bool hasMoveTarget_ = false;
	Vector3 movePosition_ = Vector3::Zero;
	Quaternion moveRotation_ = Quaternion::Identity;
	bool isMovingKinematic_ = false;

	PhysicsWorld::BodyId bodyId_ = PhysicsWorld::kInvalidBodyId;


private: // メンバ関数

	void SyncBodyPose();
	void WriteBackPose();

	void ApplyMotion();
	void PrepareMoveTarget();
};

} // namespace ou



template <>
struct EnableBitmaskOperators<ou::RigidbodyConstraints> {
	static constexpr bool enable = true;
};