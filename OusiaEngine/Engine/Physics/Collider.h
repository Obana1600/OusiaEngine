#pragma once

#include "Math/Bounds.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Physics/Box3D/PhysicsWorld.h"
#include "Scene/Behaviour.h"

#include <memory>


namespace ou {

// 前方宣言
class Rigidbody;
class PhysicsMaterial;

class Collider: public Behaviour {
	friend class Physics;

public: // メンバ関数

	virtual Vector3 ClosestPoint(const Vector3 &position) const = 0;
	Vector3 ClosestPointOnBounds(const Vector3 &position) const;

	virtual Bounds GetBounds() const = 0;

	void SetIsTrigger(bool value);
	bool IsTrigger() const;

	Rigidbody *GetAttachedRigidbody() const;

	void SetSharedMaterial(std::shared_ptr<PhysicsMaterial> material);
	const std::shared_ptr<PhysicsMaterial> &GetSharedMaterial() const;

	const std::shared_ptr<PhysicsMaterial> &GetMaterial();


protected: // メンバ関数

	void OnEnable() override;
	void OnDisable() override;

	virtual PhysicsWorld::ShapeGeometry BuildShapeGeometry() const = 0;
	virtual Vector3 GetShapeCenter() const = 0;


private: // メンバ変数

	bool isTrigger_ = false;
	std::shared_ptr<PhysicsMaterial> sharedMaterial_;
	bool isMaterialInstanced_ = false;

	PhysicsWorld::BodyId bodyId_ = PhysicsWorld::kInvalidBodyId;
	PhysicsWorld::ShapeId shapeId_ = PhysicsWorld::kInvalidShapeId;
	PhysicsWorld::ShapeGeometry geometry_;
	PhysicsWorld::SurfaceMaterial surfaceMaterial_;
	PhysicsWorld::CollisionFilter filter_;
	bool isSensorShape_ = false;

	// Rigidbody がないときに使う
	PhysicsWorld::BodyId ownBodyId_ = PhysicsWorld::kInvalidBodyId;
	Vector3 bodyPosition_ = Vector3::Zero;
	Quaternion bodyRotation_ = Quaternion::Identity;


private: // メンバ関数

	void SyncToPhysics();
	void SyncOwnBody();
	PhysicsWorld::ShapeGeometry CalculateShapeGeometry(const Rigidbody *rigidbody) const;
	PhysicsWorld::SurfaceMaterial CalculateSurfaceMaterial() const;
	void ReleaseShape();
	void ReleaseOwnBody();
};

} // namespace ou
