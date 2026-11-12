#include "CharacterController.h"
#include "Core/Time.h"
#include "Math/Mathf.h"
#include "Math/Matrix4x4.h"
#include "Physics/ControllerColliderHit.h"
#include "Physics/Physics.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"
#include "Serialize/Serializer.h"
#include "Scene/Transform.h"

#include <algorithm>

using namespace ou;


namespace {

// 地面と天井を判定する、法線の閾値
constexpr float kFlagNormalY = 0.01f;

// 段差を検知する最小の高さ
constexpr float kClimbTolerance = 0.0001f;

// 段差を上る高さの余裕
constexpr float kStepHeightTolerance = 0.01f;

// 段差を上った先の地面をレイで調べるときの高さ / 距離
constexpr float kGroundProbeHeight = 0.05f;
constexpr float kGroundProbeInset = 0.01f;

CollisionFlags ClassifyHit(const Vector3 &normal) {
	if (normal.y > kFlagNormalY) { return CollisionFlags::Below; }
	if (normal.y < -kFlagNormalY) { return CollisionFlags::Above; }
	return CollisionFlags::Sides;
}

} // namespace



void ou::CharacterController::Serialize(Serializer &serializer) {
	Collider::Serialize(serializer);

	serializer.Field("center", center_);
	serializer.Field("radius", radius_);
	serializer.Field("height", height_);
	serializer.Field("slopeLimit", slopeLimit_);
	serializer.Field("stepOffset", stepOffset_);
	serializer.Field("skinWidth", skinWidth_);
	serializer.Field("minMoveDistance", minMoveDistance_);

	if (serializer.IsReading()) {
		SetRadius(radius_);
		SetHeight(height_);
		SetSlopeLimit(slopeLimit_);
		SetStepOffset(stepOffset_);
		SetSkinWidth(skinWidth_);
		SetMinMoveDistance(minMoveDistance_);
	}
}


Vector3 ou::CharacterController::ClosestPoint(const Vector3 &position) const {
	const auto capsule = CalculateWorldCapsule();

	const auto segment = capsule.point1 - capsule.point0;
	const auto lengthSquared = segment.LengthSquared();
	const auto t = lengthSquared > 0.0f ? Mathf::Clamp01(Vector3::Dot(position - capsule.point0, segment) / lengthSquared)
										: 0.0f;
	const auto nearest = capsule.point0 + segment * t;

	const auto offset = position - nearest;
	if (offset.LengthSquared() <= capsule.radius * capsule.radius) { return position; }

	return nearest + offset.Normalized() * capsule.radius;
}


Bounds ou::CharacterController::GetBounds() const {
	const auto capsule = CalculateWorldCapsule();
	const auto extents = Vector3::One * capsule.radius;

	Bounds bounds;
	bounds.SetMinMax(Vector3::Min(capsule.point0, capsule.point1) - extents,
					 Vector3::Max(capsule.point0, capsule.point1) + extents);

	return bounds;
}


CollisionFlags ou::CharacterController::Move(const Vector3 &motion) {
	if (motion.LengthSquared() < minMoveDistance_ * minMoveDistance_) {
		velocity_ = Vector3::Zero;
		return collisionFlags_;
	}

	const auto capsule = CalculateWorldCapsule();

	const auto movesHorizontally = Vector3(motion.x, 0.0f, motion.z).LengthSquared() > 0.0f;
	const auto canStep = stepOffset_ > 0.0f && IsGrounded() && motion.y <= 0.0f && movesHorizontally;

	auto result = canStep ? MoveWithStep(capsule, motion)
						  : MovePass(capsule, Vector3::Zero, motion, !movesHorizontally);

	if (canStep && result.delta.y > kClimbTolerance && !IsWalkableStep(capsule, result)) {
		result = MovePass(capsule, Vector3::Zero, motion, false);
	}

	auto *transform = GetTransform();
	transform->SetWorldPosition(transform->GetWorldPosition() + result.delta);

	collisionFlags_ = CollisionFlags::None;
	for (const auto &hit : result.hits) {
		collisionFlags_ |= ClassifyHit(hit.normal);
	}

	const auto deltaTime = Time::GetDeltaTime();
	velocity_ = (deltaTime > 0.0f) ? result.delta / deltaTime
								   : Vector3::Zero;

	SendControllerColliderHits(result.hits, motion, result.delta.Length());
	return collisionFlags_;
}


bool ou::CharacterController::SimpleMove(const Vector3 &speed) {
	if (IsGrounded()) { fallVelocity_ = Vector3::Zero; }

	const auto deltaTime = Time::GetDeltaTime();
	fallVelocity_ += Physics::GetGravity() * deltaTime;

	Move((Vector3(speed.x, 0.0f, speed.z) + fallVelocity_) * deltaTime);
	return IsGrounded();
}


bool ou::CharacterController::IsGrounded() const {
	return (collisionFlags_ & CollisionFlags::Below) != CollisionFlags::None;
}


CollisionFlags ou::CharacterController::GetCollisionFlags() const {
	return collisionFlags_;
}


const Vector3 &ou::CharacterController::GetVelocity() const {
	return velocity_;
}


void ou::CharacterController::SetCenter(const Vector3 &center) {
	center_ = center;
}


const Vector3 &ou::CharacterController::GetCenter() const {
	return center_;
}


void ou::CharacterController::SetRadius(float radius) {
	radius_ = Mathf::Max(radius, 0.0f);
}


float ou::CharacterController::GetRadius() const {
	return radius_;
}


void ou::CharacterController::SetHeight(float height) {
	height_ = Mathf::Max(height, 0.0f);
}


float ou::CharacterController::GetHeight() const {
	return height_;
}


void ou::CharacterController::SetSlopeLimit(float slopeLimit) {
	slopeLimit_ = Mathf::Clamp(slopeLimit, 0.0f, 180.0f);
}


float ou::CharacterController::GetSlopeLimit() const {
	return slopeLimit_;
}


void ou::CharacterController::SetStepOffset(float stepOffset) {
	stepOffset_ = Mathf::Max(stepOffset, 0.0f);
}


float ou::CharacterController::GetStepOffset() const {
	return stepOffset_;
}


void ou::CharacterController::SetSkinWidth(float skinWidth) {
	skinWidth_ = Mathf::Max(skinWidth, 0.0f);
}


float ou::CharacterController::GetSkinWidth() const {
	return skinWidth_;
}


void ou::CharacterController::SetMinMoveDistance(float minMoveDistance) {
	minMoveDistance_ = Mathf::Max(minMoveDistance, 0.0f);
}


float ou::CharacterController::GetMinMoveDistance() const {
	return minMoveDistance_;
}


PhysicsWorld::ShapeGeometry ou::CharacterController::BuildShapeGeometry() const {
	return {.type = PhysicsWorld::ShapeGeometry::Type::Capsule,
			.radius = radius_,
			.height = height_,
			.direction = 1};
}


Vector3 ou::CharacterController::GetShapeCenter() const {
	return center_;
}


PhysicsWorld::MotionType ou::CharacterController::GetOwnBodyMotionType() const {
	return PhysicsWorld::MotionType::Kinematic;
}


CharacterController::WorldCapsule ou::CharacterController::CalculateWorldCapsule() const {
	const auto *transform = GetTransform();
	const auto scale = transform->GetLossyScale();

	WorldCapsule capsule{};
	capsule.radius = radius_ * Mathf::Max(Mathf::Abs(scale.x), Mathf::Abs(scale.z));
	const auto halfLength = Mathf::Max(height_ * Mathf::Abs(scale.y) * 0.5f - capsule.radius, 0.0f);

	const auto center = transform->GetLocalToWorldMatrix().MultiplyPoint3x4(center_);
	capsule.point0 = center - Vector3::Up * halfLength;
	capsule.point1 = center + Vector3::Up * halfLength;
	return capsule;
}


PhysicsWorld::CapsuleMoveResult ou::CharacterController::MovePass(const WorldCapsule &capsule, const Vector3 &offset, const Vector3 &motion, bool flattenGround) const {
	const auto layerMask = Physics::CalculateCollisionFilter(GetGameObject()->GetLayer()).maskBits;
	const auto minGroundNormalY = Mathf::Cos(slopeLimit_ * Mathf::Deg2Rad);

	return PhysicsWorld::MoveCapsule(capsule.point0 + offset, capsule.point1 + offset, capsule.radius + skinWidth_,
									 motion, layerMask, GetShapeId(), minGroundNormalY, flattenGround);
}


PhysicsWorld::CapsuleMoveResult ou::CharacterController::MoveWithStep(const WorldCapsule &capsule, const Vector3 &motion) const {
	auto up = MovePass(capsule, Vector3::Zero, Vector3::Up * stepOffset_, false);
	const auto side = MovePass(capsule, up.delta, Vector3(motion.x, 0.0f, motion.z), false);
	const auto down = MovePass(capsule, up.delta + side.delta, Vector3::Up * (motion.y - up.delta.y), true);
	
	PhysicsWorld::CapsuleMoveResult result;
	result.delta = up.delta + side.delta + down.delta;
	result.hits = std::move(up.hits);
	result.hits.insert(result.hits.end(), side.hits.begin(), side.hits.end());
	result.hits.insert(result.hits.end(), down.hits.begin(), down.hits.end());
	return result;
}


bool ou::CharacterController::IsWalkableStep(const WorldCapsule &capsule, const PhysicsWorld::CapsuleMoveResult &result) const {
	const auto layerMask = Physics::CalculateCollisionFilter(GetGameObject()->GetLayer()).maskBits;
	const auto minGroundNormalY = Mathf::Cos(slopeLimit_ * Mathf::Deg2Rad);
	const auto center = (capsule.point0 + capsule.point1) * 0.5f + result.delta;

	const auto maxStepY = capsule.point0.y - capsule.radius - skinWidth_ + stepOffset_ + kStepHeightTolerance;

	for (const auto &hit : result.hits) {
		if (hit.normal.y <= kFlagNormalY) { continue; }
		if (hit.point.y > maxStepY) { return false; }
		if (hit.normal.y >= minGroundNormalY) { continue; }

		auto inward = center - hit.point;
		inward.y = 0.0f;
		const auto origin = hit.point + inward.Normalized() * kGroundProbeInset + Vector3::Up * kGroundProbeHeight;

		for (const auto &ground : PhysicsWorld::RaycastAll(origin, Vector3::Down, kGroundProbeHeight * 2.0f, layerMask, false)) {
			if (ground.shapeId == GetShapeId()) { continue; }
			if (ground.normal.y < minGroundNormalY) { return false; }
			break;
		}
	}
	
	return true;
}


void ou::CharacterController::SendControllerColliderHits(const std::vector<PhysicsWorld::CapsuleMoveHit> &hits, const Vector3 &motion, float moveLength) {
	std::vector<PhysicsWorld::ShapeId> sentShapes;
	std::vector<ControllerColliderHit> controllerHits;
	const auto moveDirection = motion.Normalized();

	for (const auto &hit : hits) {
		if (std::ranges::find(sentShapes, hit.shapeId) != sentShapes.end()) { continue; }
		sentShapes.push_back(hit.shapeId);

		if (auto *collider = Physics::FindCollider(hit.shapeId)) {
			controllerHits.emplace_back(this, collider, hit.point, hit.normal, moveDirection, moveLength);
		}
	}

	for (const auto &controllerHit : controllerHits) {
		Physics::SendToReceivers(this, [&](Component *component) {
			component->OnControllerColliderHit(controllerHit);
		});
	}
}
