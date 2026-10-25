#include "Rigidbody.h"
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


void ou::Rigidbody::SyncBodyPose() {
	const auto *transform = GetTransform();
	auto position = transform->GetWorldPosition();
	auto rotation = transform->GetWorldRotation();
	if (position_ == position && rotation_ == rotation) { return; }

	position_ = position;
	rotation_ = rotation;
	PhysicsWorld::SetBodyPose(bodyId_, position_, rotation_);
}


void ou::Rigidbody::WriteBackPose() {
	PhysicsWorld::GetBodyPose(bodyId_, &position_, &rotation_);

	auto *transform = GetTransform();
	transform->SetWorldPosition(position_);
	transform->SetWorldRotation(rotation_);
}
