#include "ControllerColliderHit.h"
#include "Physics/Collider.h"
#include "Serialize/Serializer.h"

using namespace ou;


ou::ControllerColliderHit::ControllerColliderHit(CharacterController *controller, Collider *collider, const Vector3 &point, const Vector3 &normal, const Vector3 &moveDirection, float moveLength)
	: controller_(controller),
	  collider_(collider),
	  point_(point),
	  normal_(normal),
	  moveDirection_(moveDirection),
	  moveLength_(moveLength) {}


CharacterController *ou::ControllerColliderHit::GetController() const {
	return controller_;
}


Collider *ou::ControllerColliderHit::GetCollider() const {
	return collider_;
}


Rigidbody *ou::ControllerColliderHit::GetRigidbody() const {
	return collider_->GetAttachedRigidbody();
}


GameObject *ou::ControllerColliderHit::GetGameObject() const {
	return collider_->GetGameObject();
}


Transform *ou::ControllerColliderHit::GetTransform() const {
	return collider_->GetTransform();
}


const Vector3 &ou::ControllerColliderHit::GetPoint() const {
	return point_;
}


const Vector3 &ou::ControllerColliderHit::GetNormal() const {
	return normal_;
}


const Vector3 &ou::ControllerColliderHit::GetMoveDirection() const {
	return moveDirection_;
}


float ou::ControllerColliderHit::GetMoveLength() const {
	return moveLength_;
}
