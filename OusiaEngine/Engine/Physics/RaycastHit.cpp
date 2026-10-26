#include "RaycastHit.h"
#include "Physics/Collider.h"
#include "Physics/Rigidbody.h"

using namespace ou;


Rigidbody *ou::RaycastHit::GetRigidbody() const {
	return collider ? collider->GetAttachedRigidbody() : nullptr;
}


Transform *ou::RaycastHit::GetTransform() const {
	if (auto *rigidbody = GetRigidbody()) { return rigidbody->GetTransform(); }
	return collider ? collider->GetTransform() : nullptr;
}
