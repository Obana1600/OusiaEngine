#pragma once

#include "Math/Bounds.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Physics/Jolt/PhysicsWorld.h"
#include "Scene/Behaviour.h"


namespace ou {

// 前方宣言
class Rigidbody;

class Collider: public Behaviour {
	friend class Physics;

public: // メンバ関数

	virtual Vector3 ClosestPoint(const Vector3 &position) const = 0;
	Vector3 ClosestPointOnBounds(const Vector3 &position) const;

	virtual Bounds GetBounds() const = 0;

	void SetIsTrigger(bool value);
	bool IsTrigger() const;

	Rigidbody *GetAttachedRigidbody() const;


protected: // メンバ関数

	void OnEnable() override;
	void OnDisable() override;

	virtual PhysicsWorld::ShapeId CreateShape() const = 0;
	virtual Vector3 GetShapeCenter() const = 0;

	PhysicsWorld::ShapeId GetShapeId() const;


private: // メンバ変数

	bool isTrigger_ = false;

	Vector3 shapePosition_ = Vector3::Zero;
	Quaternion shapeRotation_ = Quaternion::Identity;
	Vector3 shapeScale_ = Vector3::One;
	Vector3 bodyPosition_ = Vector3::Zero;
	Quaternion bodyRotation_ = Quaternion::Identity;

	PhysicsWorld::ShapeId shapeId_ = PhysicsWorld::kInvalidShapeId;
	PhysicsWorld::BodyId bodyId_ = PhysicsWorld::kInvalidBodyId;

	PhysicsWorld::BodyId ownBodyId_ = PhysicsWorld::kInvalidBodyId;
	PhysicsWorld::MotionType ownMotionType_ = PhysicsWorld::MotionType::Static;
	bool isOwnBodySensor_ = false;

private: // メンバ関数

	void SyncToPhysics();
	void UpdateOwnBody(const Rigidbody *rigidbody);
	void AttachToBody(const Rigidbody *bodyOwner, PhysicsWorld::BodyId id);
	void SyncShapePose(const Rigidbody *bodyOwner);
	void CalculateShapePose(const Rigidbody *bodyOwner, Vector3 *position, Quaternion *rotation, Vector3 *scale);
	void ReleaseShape();
	void ReleaseOwnBody();
};

} // namespace ou
