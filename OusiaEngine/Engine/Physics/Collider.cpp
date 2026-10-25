#include "Collider.h"
#include "Physics/Physics.h"
#include "Physics/Rigidbody.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"

using namespace ou;


Vector3 ou::Collider::ClosestPointOnBounds(const Vector3 &position) const {
	return GetBounds().ClosestPoint(position);
}


void ou::Collider::SetIsTrigger(bool value) {
	isTrigger_ = value;
}


bool ou::Collider::IsTrigger() const {
	return isTrigger_;
}


Rigidbody *ou::Collider::GetAttachedRigidbody() const {
	for (auto *transform = GetTransform(); transform; transform = transform->GetParent()) {
		auto *rigidbody = transform->GetGameObject()->GetComponent<Rigidbody>();
		if (rigidbody && !rigidbody->IsDestroyed()) { return rigidbody; }
	}
	return nullptr;
}


void ou::Collider::OnEnable() {
	Physics::RegisterCollider(this);
}


void ou::Collider::OnDisable() {
	ReleaseShape();
	Physics::UnregisterCollider(this);
}


PhysicsWorld::ShapeId ou::Collider::GetShapeId() const {
	return shapeId_;
}


void ou::Collider::SyncToPhysics() {
	if (shapeId_ == PhysicsWorld::kInvalidShapeId) { shapeId_ = CreateShape(); }

	auto *rigidbody = GetAttachedRigidbody();
	const auto *bodyOwner = isTrigger_ ? nullptr : rigidbody;
	if (bodyOwner) {
		ReleaseOwnBody();
	} else {
		UpdateOwnBody(rigidbody);
	}

	const auto targetBodyId = bodyOwner ? bodyOwner->bodyId_ : ownBodyId_;
	if (targetBodyId == PhysicsWorld::kInvalidBodyId || targetBodyId != bodyId_) {
		AttachToBody(bodyOwner, targetBodyId);
		return;
	}

	SyncShapePose(bodyOwner);
}


void ou::Collider::UpdateOwnBody(const Rigidbody *rigidbody) {
	const auto motionType = rigidbody ? PhysicsWorld::MotionType::Kinematic
									  : PhysicsWorld::MotionType::Static;

	if (ownBodyId_ == PhysicsWorld::kInvalidBodyId) {
		const auto *transform = GetTransform();
		bodyPosition_ = transform->GetWorldPosition();
		bodyRotation_ = transform->GetWorldRotation();
		ownBodyId_ = PhysicsWorld::CreateBody(bodyPosition_, bodyRotation_, motionType);
		ownMotionType_ = motionType;
		isOwnBodySensor_ = false;
	}

	if (ownMotionType_ != motionType) {
		ownMotionType_ = motionType;
		PhysicsWorld::SetMotionType(ownBodyId_, ownMotionType_);
	}
	if (isOwnBodySensor_ != isTrigger_) {
		isOwnBodySensor_ = isTrigger_;
		PhysicsWorld::SetIsSensor(ownBodyId_, isOwnBodySensor_);
	}
}


void ou::Collider::AttachToBody(const Rigidbody *bodyOwner, PhysicsWorld::BodyId id) {
	bodyId_ = id;
	CalculateShapePose(bodyOwner, &shapePosition_, &shapeRotation_, &shapeScale_);
	PhysicsWorld::AttachShape(bodyId_, shapeId_, shapePosition_, shapeRotation_, shapeScale_);
}


void ou::Collider::SyncShapePose(const Rigidbody *bodyOwner) {
	if (!bodyOwner) {
		const auto *transform = GetTransform();
		auto position = transform->GetWorldPosition();
		auto rotation = transform->GetWorldRotation();
		if (position != bodyPosition_ || rotation != bodyRotation_) {
			bodyPosition_ = position;
			bodyRotation_ = rotation;
			PhysicsWorld::SetBodyPose(ownBodyId_, bodyPosition_, bodyRotation_);
		}
	}

	Vector3 position;
	Quaternion rotation;
	Vector3 scale;
	CalculateShapePose(bodyOwner, &position, &rotation, &scale);
	if (position == shapePosition_ && rotation == shapeRotation_ && scale == shapeScale_) { return; }

	shapePosition_ = position;
	shapeRotation_ = rotation;
	shapeScale_ = scale;
	PhysicsWorld::SetShapeLocalPose(shapeId_, shapePosition_, shapeRotation_, shapeScale_);
}


void ou::Collider::CalculateShapePose(const Rigidbody *bodyOwner, Vector3 *position, Quaternion *rotation, Vector3 *scale) {
	const auto *transform = GetTransform();
	*scale = transform->GetLossyScale();

	const auto *owner = bodyOwner ? bodyOwner->GetTransform() : transform;
	if (owner == transform) {
		*position = Vector3::Scale(GetShapeCenter(), *scale);
		*rotation = Quaternion::Identity;
		return;
	}

	const auto inverse = Quaternion::Inverse(owner->GetWorldRotation());
	const auto center = transform->GetLocalToWorldMatrix().MultiplyPoint3x4(GetShapeCenter());
	*position = inverse * (center - owner->GetWorldPosition());
	*rotation = inverse * transform->GetWorldRotation();
}


void ou::Collider::ReleaseShape() {
	if (shapeId_ != PhysicsWorld::kInvalidShapeId) {
		PhysicsWorld::DestroyShape(shapeId_);
		shapeId_ = PhysicsWorld::kInvalidShapeId;
	}

	ReleaseOwnBody();
	bodyId_ = PhysicsWorld::kInvalidBodyId;
}


void ou::Collider::ReleaseOwnBody() {
	if (ownBodyId_ == PhysicsWorld::kInvalidBodyId) { return; }

	PhysicsWorld::DestroyBody(ownBodyId_);
	ownBodyId_ = PhysicsWorld::kInvalidBodyId;
}
