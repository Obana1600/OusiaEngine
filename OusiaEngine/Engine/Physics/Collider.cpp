#include "Collider.h"
#include "Physics/Physics.h"

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


void ou::Collider::OnEnable() {
	Physics::RegisterCollider(this);
}


void ou::Collider::OnDisable() {
	Physics::UnregisterCollider(this);
}
