#include "Collider.h"
#include "Physics/Physics.h"
#include "Physics/PhysicsMaterial.h"
#include "Physics/Rigidbody.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"

#include <string>
#include <utility>

using namespace ou;


namespace {

PhysicsWorld::CombineMode ToCombineMode(PhysicsMaterialCombine combine) {
	switch (combine) {
		case PhysicsMaterialCombine::Minimum:
			return PhysicsWorld::CombineMode::Minimum;
		case PhysicsMaterialCombine::Multiply:
			return PhysicsWorld::CombineMode::Multiply;
		case PhysicsMaterialCombine::Maximum:
			return PhysicsWorld::CombineMode::Maximum;
		default:
			return PhysicsWorld::CombineMode::Average;
	}
}

} // namespace



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


void ou::Collider::SetSharedMaterial(std::shared_ptr<PhysicsMaterial> material) {
	sharedMaterial_ = std::move(material);
	isMaterialInstanced_ = false;
}


const std::shared_ptr<PhysicsMaterial> &ou::Collider::GetSharedMaterial() const {
	return sharedMaterial_;
}


const std::shared_ptr<PhysicsMaterial> &ou::Collider::GetMaterial() {
	if (isMaterialInstanced_) { return sharedMaterial_; }

	auto instance = sharedMaterial_ ? PhysicsMaterial::Create(*sharedMaterial_)
									: std::make_shared<PhysicsMaterial>();
	const std::string name = sharedMaterial_ ? sharedMaterial_->GetName()
											 : "Default";
	instance->SetName(name + " (Instance)");

	sharedMaterial_ = std::move(instance);
	isMaterialInstanced_ = true;
	return sharedMaterial_;
}


void ou::Collider::OnEnable() {
	Physics::RegisterCollider(this);
}


void ou::Collider::OnDisable() {
	ReleaseShape();
	ReleaseOwnBody();
	Physics::UnregisterCollider(this);
}


void ou::Collider::SyncToPhysics() {
	auto *rigidbody = GetAttachedRigidbody();
	if (rigidbody) {
		ReleaseOwnBody();
	} else {
		SyncOwnBody();
	}

	const auto bodyId = rigidbody ? rigidbody->bodyId_ : ownBodyId_;
	const auto geometry = CalculateShapeGeometry(rigidbody);
	const auto surfaceMaterial = CalculateSurfaceMaterial();
	const auto filter = Physics::CalculateCollisionFilter(GetGameObject()->GetLayer());

	if (shapeId_ == PhysicsWorld::kInvalidShapeId || bodyId != bodyId_ || isTrigger_ != isSensorShape_) {
		ReleaseShape();
		bodyId_ = bodyId;
		geometry_ = geometry;
		surfaceMaterial_ = surfaceMaterial;
		filter_ = filter;
		isSensorShape_ = isTrigger_;
		shapeId_ = PhysicsWorld::CreateShape(bodyId_, geometry_, surfaceMaterial_, filter_, isSensorShape_);
		return;
	}

	if (geometry != geometry_) {
		geometry_ = geometry;
		PhysicsWorld::SetShapeGeometry(shapeId_, geometry_);
	}

	if (surfaceMaterial_ != surfaceMaterial) {
		surfaceMaterial_ = surfaceMaterial;
		PhysicsWorld::SetShapeSurfaceMaterial(shapeId_, surfaceMaterial_);
	}

	if (filter_ != filter) {
		filter_ = filter;
		PhysicsWorld::SetShapeFilter(shapeId_, filter_);
	}
}


void ou::Collider::SyncOwnBody() {
	const auto *transform = GetTransform();
	const auto position = transform->GetWorldPosition();
	const auto rotation = transform->GetWorldRotation();

	// まだ Body がないなら作る
	if (ownBodyId_ == PhysicsWorld::kInvalidBodyId) {
		bodyPosition_ = position;
		bodyRotation_ = rotation;
		ownBodyId_ = PhysicsWorld::CreateBody(bodyPosition_, bodyRotation_, PhysicsWorld::MotionType::Static);
		return;
	}

	// Body の更新
	if (position != bodyPosition_ || rotation != bodyRotation_) {
		bodyPosition_ = position;
		bodyRotation_ = rotation;
		PhysicsWorld::SetBodyPose(ownBodyId_, bodyPosition_, bodyRotation_);
	}
}


PhysicsWorld::ShapeGeometry ou::Collider::CalculateShapeGeometry(const Rigidbody *rigidbody) const {
	auto geometry = BuildShapeGeometry();

	const auto *transform = GetTransform();
	geometry.scale = transform->GetLossyScale();

	const auto *owner = rigidbody ? rigidbody->GetTransform() : transform;
	if (owner == transform) {
		geometry.localPosition = Vector3::Scale(GetShapeCenter(), geometry.scale);
		geometry.localRotation = Quaternion::Identity;
		return geometry;
	}

	const auto inverse = Quaternion::Inverse(owner->GetWorldRotation());
	const auto center = transform->GetLocalToWorldMatrix().MultiplyPoint3x4(GetShapeCenter());
	geometry.localPosition = inverse * (center - owner->GetWorldPosition());
	geometry.localRotation = inverse * transform->GetWorldRotation();
	return geometry;
}


PhysicsWorld::SurfaceMaterial ou::Collider::CalculateSurfaceMaterial() const {
	if (!sharedMaterial_) { return {}; }

	return {sharedMaterial_->GetDynamicFriction(), sharedMaterial_->GetBounciness(),
			ToCombineMode(sharedMaterial_->GetFrictionCombine()), ToCombineMode(sharedMaterial_->GetBounceCombine())};
}


void ou::Collider::ReleaseShape() {
	PhysicsWorld::DestroyShape(shapeId_);
	shapeId_ = PhysicsWorld::kInvalidShapeId;
	bodyId_ = PhysicsWorld::kInvalidBodyId;
}


void ou::Collider::ReleaseOwnBody() {
	if (ownBodyId_ == PhysicsWorld::kInvalidBodyId) { return; }

	PhysicsWorld::DestroyBody(ownBodyId_);
	ownBodyId_ = PhysicsWorld::kInvalidBodyId;
}
