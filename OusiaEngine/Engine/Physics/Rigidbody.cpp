#include "Rigidbody.h"
#include "Core/Time.h"
#include "Math/Mathf.h"
#include "Physics/Collider.h"
#include "Physics/Physics.h"
#include "Scene/Transform.h"

using namespace ou;


namespace {

// 質量の下限
constexpr float kMinMass = 1e-7f;

} // namespace



void ou::Rigidbody::Awake() {
	const auto *transform = GetTransform();
	position_ = previousPosition_ = writtenPosition_ = transform->GetWorldPosition();
	rotation_ = previousRotation_ = writtenRotation_ = transform->GetWorldRotation();

	bodyId_ = PhysicsWorld::CreateBody(position_, rotation_, PhysicsWorld::MotionType::Dynamic);
	PhysicsWorld::SetUseGravity(bodyId_, useGravity_);
	PhysicsWorld::SetLinearDamping(bodyId_, linearDamping_);
	PhysicsWorld::SetAngularDamping(bodyId_, angularDamping_);
	PhysicsWorld::SetSleepThreshold(bodyId_, sleepThreshold_);
	PhysicsWorld::SetBullet(bodyId_, collisionDetectionMode_ == CollisionDetectionMode::ContinuousDynamic);
	ApplyMotion();

	Physics::RegisterRigidbody(this);
}


void ou::Rigidbody::OnDestroy() {
	Physics::UnregisterRigidbody(this);

	PhysicsWorld::DestroyBody(bodyId_);
	bodyId_ = PhysicsWorld::kInvalidBodyId;
}


void ou::Rigidbody::SetMass(float mass) {
	mass_ = Mathf::Max(mass, kMinMass);
	PhysicsWorld::SetMass(bodyId_, mass_);
}


float ou::Rigidbody::GetMass() const {
	return mass_;
}


void ou::Rigidbody::SetUseGravity(bool useGravity) {
	useGravity_ = useGravity;
	PhysicsWorld::SetUseGravity(bodyId_, useGravity_);
}


bool ou::Rigidbody::GetUseGravity() const {
	return useGravity_;
}


void ou::Rigidbody::SetIsKinematic(bool isKinematic) {
	isKinematic_ = isKinematic;
	ApplyMotion();
}


bool ou::Rigidbody::IsKinematic() const {
	return isKinematic_;
}


void ou::Rigidbody::SetLinearDamping(float damping) {
	linearDamping_ = Mathf::Max(damping, 0.0f);
	PhysicsWorld::SetLinearDamping(bodyId_, linearDamping_);
}


float ou::Rigidbody::GetLinearDamping() const {
	return linearDamping_;
}


void ou::Rigidbody::SetAngularDamping(float damping) {
	angularDamping_ = Mathf::Max(damping, 0.0f);
	PhysicsWorld::SetAngularDamping(bodyId_, angularDamping_);
}


float ou::Rigidbody::GetAngularDamping() const {
	return angularDamping_;
}


void ou::Rigidbody::SetMaxLinearVelocity(float velocity) {
	maxLinearVelocity_ = Mathf::Max(velocity, 0.0f);
}


float ou::Rigidbody::GetMaxLinearVelocity() const {
	return maxLinearVelocity_;
}


void ou::Rigidbody::SetMaxAngularVelocity(float velocity) {
	maxAngularVelocity_ = Mathf::Max(velocity, 0.0f);
}


float ou::Rigidbody::GetMaxAngularVelocity() const {
	return maxAngularVelocity_;
}


void ou::Rigidbody::SetSleepThreshold(float threshold) {
	sleepThreshold_ = Mathf::Max(threshold, 0.0f);
	PhysicsWorld::SetSleepThreshold(bodyId_, sleepThreshold_);
}


float ou::Rigidbody::GetSleepThreshold() const {
	return sleepThreshold_;
}


void ou::Rigidbody::SetCollisionDetectionMode(CollisionDetectionMode mode) {
	collisionDetectionMode_ = mode;
	PhysicsWorld::SetBullet(bodyId_, collisionDetectionMode_ == CollisionDetectionMode::ContinuousDynamic);
}


CollisionDetectionMode ou::Rigidbody::GetCollisionDetectionMode() const {
	return collisionDetectionMode_;
}


void ou::Rigidbody::SetConstraints(RigidbodyConstraints constraints) {
	constraints_ = constraints;
	ApplyMotion();
}


RigidbodyConstraints ou::Rigidbody::GetConstraints() const {
	return constraints_;
}


void ou::Rigidbody::SetInterpolation(RigidbodyInterpolation interpolation) {
	interpolation_ = interpolation;

	previousPosition_ = position_;
	previousRotation_ = rotation_;
}


RigidbodyInterpolation ou::Rigidbody::GetInterpolation() const {
	return interpolation_;
}


void ou::Rigidbody::SetLinearVelocity(const Vector3 &velocity) {
	PhysicsWorld::SetLinearVelocity(bodyId_, velocity);
}


Vector3 ou::Rigidbody::GetLinearVelocity() const {
	return PhysicsWorld::GetLinearVelocity(bodyId_);
}


void ou::Rigidbody::SetAngularVelocity(const Vector3 &velocity) {
	PhysicsWorld::SetAngularVelocity(bodyId_, velocity);
}


Vector3 ou::Rigidbody::GetAngularVelocity() const {
	return PhysicsWorld::GetAngularVelocity(bodyId_);
}


void ou::Rigidbody::AddForce(const Vector3 &force, ForceMode mode) {
	switch (mode) {
		case ForceMode::Force:
			PhysicsWorld::AddForce(bodyId_, force);
			break;
		case ForceMode::Acceleration:
			PhysicsWorld::AddForce(bodyId_, force * mass_);
			break;
		case ForceMode::Impulse:
			PhysicsWorld::AddImpulse(bodyId_, force);
			break;
		case ForceMode::VelocityChange:
			PhysicsWorld::AddImpulse(bodyId_, force * mass_);
			break;
	}
}


void ou::Rigidbody::AddTorque(const Vector3 &torque, ForceMode mode) {
	if (isKinematic_) { return; }

	switch (mode) {
		case ForceMode::Force:
			PhysicsWorld::AddTorque(bodyId_, torque);
			break;
		case ForceMode::Acceleration:
			SetAngularVelocity(GetAngularVelocity() + torque * Time::GetFixedDeltaTime());
			break;
		case ForceMode::Impulse:
			PhysicsWorld::AddAngularImpulse(bodyId_, torque);
			break;
		case ForceMode::VelocityChange:
			SetAngularVelocity(GetAngularVelocity() + torque);
			break;
	}
}


void ou::Rigidbody::AddRelativeForce(const Vector3 &force, ForceMode mode) {
	AddForce(rotation_ * force, mode);
}


void ou::Rigidbody::AddRelativeTorque(const Vector3 &torque, ForceMode mode) {
	AddTorque(rotation_ * torque, mode);
}


void ou::Rigidbody::AddForceAtPosition(const Vector3 &force, const Vector3 &position, ForceMode mode) {
	switch (mode) {
		case ForceMode::Force:
			PhysicsWorld::AddForceAtPoint(bodyId_, force, position);
			break;
		case ForceMode::Acceleration:
			PhysicsWorld::AddForceAtPoint(bodyId_, force * mass_, position);
			break;
		case ForceMode::Impulse:
			PhysicsWorld::AddImpulseAtPoint(bodyId_, force, position);
			break;
		case ForceMode::VelocityChange:
			PhysicsWorld::AddImpulseAtPoint(bodyId_, force * mass_, position);
			break;
	}
}


void ou::Rigidbody::AddExplosionForce(float explosionForce, const Vector3 &explosionPosition, float explosionRadius, float upwardsModifier, ForceMode mode) {
	if (isKinematic_) { return; }

	Vector3 point;
	if (!FindClosestSurfacePoint(explosionPosition, &point)) {
		point = PhysicsWorld::GetWorldCenterOfMass(bodyId_);
	}

	const auto distance = Vector3::Distance(point, explosionPosition);
	if (explosionRadius > 0.0f && distance > explosionRadius) { return; }
	const auto falloff = (explosionRadius > 0.0f) ? 1.0f - distance / explosionRadius
												  : 1.0f;

	const auto direction = (point - (explosionPosition - Vector3::Up * upwardsModifier)).Normalized();
	AddForceAtPosition(direction * (explosionForce * falloff), point, mode);
}


Vector3 ou::Rigidbody::GetPointVelocity(const Vector3 &worldPoint) const {
	return PhysicsWorld::GetPointVelocity(bodyId_, worldPoint);
}


void ou::Rigidbody::Sleep() {
	PhysicsWorld::SetAwake(bodyId_, false);
}


void ou::Rigidbody::WakeUp() {
	PhysicsWorld::SetAwake(bodyId_, true);
}


bool ou::Rigidbody::IsSleeping() const {
	return !PhysicsWorld::IsAwake(bodyId_);
}


void ou::Rigidbody::MovePosition(const Vector3 &position) {
	PrepareMoveTarget();
	movePosition_ = position;
}


void ou::Rigidbody::MoveRotation(const Quaternion &rotation) {
	PrepareMoveTarget();
	moveRotation_ = rotation;
}


Vector3 ou::Rigidbody::GetPosition() const {
	return position_;
}


Quaternion ou::Rigidbody::GetRotation() const {
	return rotation_;
}


void ou::Rigidbody::SetCenterOfMass(const Vector3 &center) {
	PhysicsWorld::SetLocalCenterOfMass(bodyId_, center);
}


void ou::Rigidbody::ResetCenterOfMass() {
	PhysicsWorld::ResetLocalCenterOfMass(bodyId_);
}


Vector3 ou::Rigidbody::GetCenterOfMass() const {
	return PhysicsWorld::GetLocalCenterOfMass(bodyId_);
}


Vector3 ou::Rigidbody::GetWorldCenterOfMass() const {
	return PhysicsWorld::GetWorldCenterOfMass(bodyId_);
}


void ou::Rigidbody::SyncBodyPose() {
	SyncMovedTransform();

	if (!hasMoveTarget_) { return; }
	hasMoveTarget_ = false;

	isMovingKinematic_ = isKinematic_;
	if (isMovingKinematic_) {
		PhysicsWorld::MoveKinematic(bodyId_, movePosition_, moveRotation_, Time::GetFixedDeltaTime());
	} else {
		PhysicsWorld::SetBodyPose(bodyId_, movePosition_, moveRotation_);
	}
}


void ou::Rigidbody::WriteBackPose() {
	previousPosition_ = position_;
	previousRotation_ = rotation_;
	PhysicsWorld::GetBodyPose(bodyId_, &position_, &rotation_);

	linearVelocity_ = PhysicsWorld::GetLinearVelocity(bodyId_);
	angularVelocity_ = PhysicsWorld::GetAngularVelocity(bodyId_);

	if (isMovingKinematic_) {
		isMovingKinematic_ = false;
		PhysicsWorld::SetLinearVelocity(bodyId_, Vector3::Zero);
		PhysicsWorld::SetAngularVelocity(bodyId_, Vector3::Zero);
	}

	if (interpolation_ == RigidbodyInterpolation::None) {
		WriteTransform(position_, rotation_);
	}
}


void ou::Rigidbody::UpdateInterpolatedPose(float alpha, float elapsed) {
	if (interpolation_ == RigidbodyInterpolation::None) { return; }

	SyncMovedTransform();

	if (interpolation_ == RigidbodyInterpolation::Interpolate) {
		WriteTransform(Vector3::Lerp(previousPosition_, position_, alpha),
					   Quaternion::Slerp(previousRotation_, rotation_, alpha));
		return;
	}

	auto rotation = rotation_;
	const auto angle = angularVelocity_.Length() * elapsed;
	if (angle > Mathf::Epsilon) {
		rotation = Quaternion::AngleAxis(angularVelocity_, angle) * rotation_;
	}
	WriteTransform(position_ + linearVelocity_ * elapsed, rotation);
}


void ou::Rigidbody::SyncMovedTransform() {
	const auto *transform = GetTransform();
	const auto position = transform->GetWorldPosition();
	const auto rotation = transform->GetWorldRotation();
	if (position == writtenPosition_ && rotation == writtenRotation_) { return; }

	position_ = previousPosition_ = writtenPosition_ = position;
	rotation_ = previousRotation_ = writtenRotation_ = rotation;
	PhysicsWorld::SetBodyPose(bodyId_, position_, rotation_);
}


void ou::Rigidbody::WriteTransform(const Vector3 &position, const Quaternion &rotation) {
	auto *transform = GetTransform();
	transform->SetWorldPosition(position);
	transform->SetWorldRotation(rotation);

	writtenPosition_ = transform->GetWorldPosition();
	writtenRotation_ = transform->GetWorldRotation();
}


void ou::Rigidbody::ApplyMotion() {
	const auto frozen = static_cast<uint8_t>(static_cast<uint8_t>(constraints_) >> 1);
	const auto dofs = static_cast<uint8_t>(PhysicsWorld::DOF::All & ~frozen);

	const auto isFrozenAll = (dofs == 0);
	PhysicsWorld::SetMotionType(bodyId_,
								isKinematic_ || isFrozenAll ? PhysicsWorld::MotionType::Kinematic
															: PhysicsWorld::MotionType::Dynamic);
	PhysicsWorld::SetMass(bodyId_, mass_);
	PhysicsWorld::SetAllowedDOFs(bodyId_, isFrozenAll ? PhysicsWorld::DOF::All : dofs);
}


void ou::Rigidbody::PrepareMoveTarget() {
	if (hasMoveTarget_) { return; }
	hasMoveTarget_ = true;
	PhysicsWorld::GetBodyPose(bodyId_, &movePosition_, &moveRotation_);
}


bool ou::Rigidbody::FindClosestSurfacePoint(const Vector3 &position, Vector3 *point) const {
	auto found = false;
	auto nearestDistanceSquared = Mathf::Infinity;

	for (const auto *collider : Physics::sColliders_) {
		if (collider->GetAttachedRigidbody() != this || collider->IsTrigger()) { continue; }

		const auto closest = collider->ClosestPoint(position);
		const auto distanceSquared = (closest - position).LengthSquared();
		if (distanceSquared <= 0.0f) { return false; }

		if (distanceSquared < nearestDistanceSquared) {
			nearestDistanceSquared = distanceSquared;
			*point = closest;
			found = true;
		}
	}

	return found;
}


void ou::Rigidbody::ClampVelocity() {
	if (isKinematic_) { return; }

	const auto linear = PhysicsWorld::GetLinearVelocity(bodyId_);
	if (linear.LengthSquared() > maxLinearVelocity_ * maxLinearVelocity_) {
		PhysicsWorld::SetLinearVelocity(bodyId_, linear.Normalized() * maxLinearVelocity_);
	}

	const auto angular = PhysicsWorld::GetAngularVelocity(bodyId_);
	if (angular.LengthSquared() > maxAngularVelocity_ * maxAngularVelocity_) {
		PhysicsWorld::SetAngularVelocity(bodyId_, angular.Normalized() * maxAngularVelocity_);
	}
}


void ou::Rigidbody::StoreVelocityBeforeStep() {
	linearVelocityBeforeStep_ = PhysicsWorld::GetLinearVelocity(bodyId_);
}
