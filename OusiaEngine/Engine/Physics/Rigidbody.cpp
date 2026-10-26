#include "Rigidbody.h"
#include "Core/Time.h"
#include "Math/Mathf.h"
#include "Physics/Physics.h"
#include "Scene/Transform.h"

using namespace ou;


namespace {

// 質量の下限
constexpr float kMinMass = 1e-7f;

} // namespace



void ou::Rigidbody::Awake() {
	const auto *transform = GetTransform();
	position_ = transform->GetWorldPosition();
	rotation_ = transform->GetWorldRotation();

	bodyId_ = PhysicsWorld::CreateBody(position_, rotation_, PhysicsWorld::MotionType::Dynamic);
	PhysicsWorld::SetMass(bodyId_, mass_);
	PhysicsWorld::SetUseGravity(bodyId_, useGravity_);
	PhysicsWorld::SetLinearDamping(bodyId_, linearDamping_);
	PhysicsWorld::SetAngularDamping(bodyId_, angularDamping_);
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


void ou::Rigidbody::SetConstraints(RigidbodyConstraints constraints) {
	constraints_ = constraints;
	ApplyMotion();
}


RigidbodyConstraints ou::Rigidbody::GetConstraints() const {
	return constraints_;
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


void ou::Rigidbody::MovePosition(const Vector3 &position) {
	PrepareMoveTarget();
	movePosition_ = position;
}


void ou::Rigidbody::MoveRotation(const Quaternion &rotation) {
	PrepareMoveTarget();
	moveRotation_ = rotation;
}


void ou::Rigidbody::SyncBodyPose() {
	const auto *transform = GetTransform();
	auto position = transform->GetWorldPosition();
	auto rotation = transform->GetWorldRotation();
	if (position_ != position || rotation_ != rotation) {
		position_ = position;
		rotation_ = rotation;
		PhysicsWorld::SetBodyPose(bodyId_, position_, rotation_);
	}

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
	if (isMovingKinematic_) {
		isMovingKinematic_ = false;
		PhysicsWorld::SetLinearVelocity(bodyId_, Vector3::Zero);
		PhysicsWorld::SetAngularVelocity(bodyId_, Vector3::Zero);
	}

	PhysicsWorld::GetBodyPose(bodyId_, &position_, &rotation_);

	auto *transform = GetTransform();
	transform->SetWorldPosition(position_);
	transform->SetWorldRotation(rotation_);
}


void ou::Rigidbody::ApplyMotion() {
	const auto frozen = static_cast<uint8_t>(static_cast<uint8_t>(constraints_) >> 1);
	const auto dofs = static_cast<uint8_t>(PhysicsWorld::DOF::All & ~frozen);

	const auto isFrozenAll = (dofs == 0);
	PhysicsWorld::SetMotionType(bodyId_,
								isKinematic_ || isFrozenAll ? PhysicsWorld::MotionType::Kinematic
															: PhysicsWorld::MotionType::Dynamic);
	PhysicsWorld::SetAllowedDOFs(bodyId_, isFrozenAll ? PhysicsWorld::DOF::All : dofs);
}


void ou::Rigidbody::PrepareMoveTarget() {
	if (hasMoveTarget_) { return; }
	hasMoveTarget_ = true;
	PhysicsWorld::GetBodyPose(bodyId_, &movePosition_, &moveRotation_);
}
