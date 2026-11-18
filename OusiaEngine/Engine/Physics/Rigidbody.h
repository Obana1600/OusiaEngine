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

enum class CollisionDetectionMode {
	Discrete,
	Continuous,
	ContinuousDynamic,
	ContinuousSpeculative,
};

enum class RigidbodyInterpolation {
	None,
	Interpolate,
	Extrapolate,
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
	void Serialize(Serializer &serializer) override;

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

	void SetMaxLinearVelocity(float velocity);
	float GetMaxLinearVelocity() const;

	void SetMaxAngularVelocity(float velocity);
	float GetMaxAngularVelocity() const;

	void SetSleepThreshold(float threshold);
	float GetSleepThreshold() const;

	void SetCollisionDetectionMode(CollisionDetectionMode mode);
	CollisionDetectionMode GetCollisionDetectionMode() const;

	void SetInterpolation(RigidbodyInterpolation interpolation);
	RigidbodyInterpolation GetInterpolation() const;

	void SetConstraints(RigidbodyConstraints constraints);
	RigidbodyConstraints GetConstraints() const;

	void SetLinearVelocity(const Vector3 &velocity);
	Vector3 GetLinearVelocity() const;

	void SetAngularVelocity(const Vector3 &velocity);
	Vector3 GetAngularVelocity() const;

	void AddForce(const Vector3 &force, ForceMode mode = ForceMode::Force);
	void AddTorque(const Vector3 &torque, ForceMode mode = ForceMode::Force);
	void AddRelativeForce(const Vector3 &force, ForceMode mode = ForceMode::Force);
	void AddRelativeTorque(const Vector3 &torque, ForceMode mode = ForceMode::Force);
	void AddForceAtPosition(const Vector3 &force, const Vector3 &position, ForceMode mode = ForceMode::Force);
	void AddExplosionForce(float explosionForce, const Vector3 &explosionPosition, float explosionRadius,
						   float upwardsModifier = 0.0f, ForceMode mode = ForceMode::Force);
	Vector3 GetPointVelocity(const Vector3 &worldPoint) const;

	void Sleep();
	void WakeUp();
	bool IsSleeping() const;

	void MovePosition(const Vector3 &position);
	void MoveRotation(const Quaternion &rotation);

	Vector3 GetPosition() const;
	Quaternion GetRotation() const;

	void SetCenterOfMass(const Vector3 &center);
	void ResetCenterOfMass();
	Vector3 GetCenterOfMass() const;
	Vector3 GetWorldCenterOfMass() const;


protected: // メンバ関数

	void AwakeFromLoad() override;


private: // メンバ変数

	float mass_ = 1.0f;
	bool useGravity_ = true;
	bool isKinematic_ = false;
	float linearDamping_ = 0.0f;
	float angularDamping_ = 0.05f;
	float maxLinearVelocity_ = 1e16f;
	float maxAngularVelocity_ = 50.0f;
	float sleepThreshold_ = 0.05f;
	CollisionDetectionMode collisionDetectionMode_ = CollisionDetectionMode::Discrete;
	RigidbodyConstraints constraints_ = RigidbodyConstraints::None;
	RigidbodyInterpolation interpolation_ = RigidbodyInterpolation::None;

	Vector3 position_ = Vector3::Zero;
	Quaternion rotation_ = Quaternion::Identity;
	Vector3 linearVelocity_ = Vector3::Zero;
	Vector3 linearVelocityBeforeStep_ = Vector3::Zero;
	Vector3 angularVelocity_ = Vector3::Zero;

	Vector3 previousPosition_ = Vector3::Zero;
	Quaternion previousRotation_ = Quaternion::Identity;

	Vector3 writtenPosition_ = Vector3::Zero;
	Quaternion writtenRotation_ = Quaternion::Identity;

	bool hasMoveTarget_ = false;
	Vector3 movePosition_ = Vector3::Zero;
	Quaternion moveRotation_ = Quaternion::Identity;
	bool isMovingKinematic_ = false;

	PhysicsWorld::BodyId bodyId_ = PhysicsWorld::kInvalidBodyId;


private: // メンバ関数

	void SyncBodyPose();
	void WriteBackPose();

	void UpdateInterpolatedPose(float alpha, float elapsed);

	void SyncMovedTransform();
	void WriteTransform(const Vector3 &position, const Quaternion &rotation);

	void ApplyMotion();
	void PrepareMoveTarget();

	bool FindClosestSurfacePoint(const Vector3 &position, Vector3 *point) const;

	void ClampVelocity();
	void StoreVelocityBeforeStep();
};

} // namespace ou



template <>
struct EnableBitmaskOperators<ou::RigidbodyConstraints> {
	static constexpr bool enable = true;
};