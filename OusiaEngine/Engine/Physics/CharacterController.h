#pragma once

#include "Core/EnumFlags.h"
#include "Math/Bounds.h"
#include "Math/Vector3.h"
#include "Physics/Collider.h"

#include <cstdint>
#include <vector>


namespace ou {

enum class CollisionFlags : uint8_t {
	None = 0,
	Sides = 1 << 0,
	Above = 1 << 1,
	Below = 1 << 2,
};

class CharacterController final: public Collider {
public: // メンバ関数

	void Serialize(Serializer &serializer) override;
	void OnDrawGizmosSelected() override;

	Vector3 ClosestPoint(const Vector3 &position) const override;
	Bounds GetBounds() const override;

	CollisionFlags Move(const Vector3 &motion);

	bool SimpleMove(const Vector3 &speed);

	bool IsGrounded() const;
	CollisionFlags GetCollisionFlags() const;
	const Vector3 &GetVelocity() const;

	void SetCenter(const Vector3 &center);
	const Vector3 &GetCenter() const;

	void SetRadius(float radius);
	float GetRadius() const;

	void SetHeight(float height);
	float GetHeight() const;

	void SetSlopeLimit(float slopeLimit);
	float GetSlopeLimit() const;

	void SetStepOffset(float stepOffset);
	float GetStepOffset() const;

	void SetSkinWidth(float skinWidth);
	float GetSkinWidth() const;

	void SetMinMoveDistance(float minMoveDistance);
	float GetMinMoveDistance() const;


protected: // メンバ関数

	void AwakeFromLoad() override;


private: // インナークラス

	struct WorldCapsule {
		Vector3 point0;
		Vector3 point1;
		float radius;
	};


private: // メンバ変数

	Vector3 center_ = Vector3::Zero;
	float radius_ = 0.5f;
	float height_ = 2.0f;
	float slopeLimit_ = 45.0f;
	float stepOffset_ = 0.3f;
	float skinWidth_ = 0.08f;
	float minMoveDistance_ = 0.001f;

	CollisionFlags collisionFlags_ = CollisionFlags::None;

	Vector3 velocity_ = Vector3::Zero;
	Vector3 fallVelocity_ = Vector3::Zero;


private: // メンバ関数

	PhysicsWorld::ShapeGeometry BuildShapeGeometry() const override;
	Vector3 GetShapeCenter() const override;
	PhysicsWorld::MotionType GetOwnBodyMotionType() const override;

	WorldCapsule CalculateWorldCapsule() const;

	PhysicsWorld::CapsuleMoveResult MovePass(const WorldCapsule &capsule, const Vector3 &offset,
											 const Vector3 &motion, bool flattenGround) const;
	PhysicsWorld::CapsuleMoveResult MoveWithStep(const WorldCapsule &capsule, const Vector3 &motion) const;
	bool IsWalkableStep(const WorldCapsule &capsule, const PhysicsWorld::CapsuleMoveResult &result) const;
	void SendControllerColliderHits(const std::vector<PhysicsWorld::CapsuleMoveHit> &hits,
									const Vector3 &motion, float moveLength);
};

} // namespace ou



template <>
struct EnableBitmaskOperators<ou::CollisionFlags> {
	static constexpr bool enable = true;
};
