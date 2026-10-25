#include "Collision.h"
#include "Physics/Collider.h"
#include "Physics/Rigidbody.h"

#include <utility>

using namespace ou;


ou::Collision::Collision(Collider *collider, Rigidbody *rigidbody, std::vector<ContactPoint> contacts)
	: collider_(collider),
	  rigidbody_(rigidbody),
	  contacts_(std::move(contacts)) {}


Collider *ou::Collision::GetCollider() const {
	return collider_;
}


Rigidbody *ou::Collision::GetRigidbody() const {
	return rigidbody_;
}


GameObject *ou::Collision::GetGameObject() const {
	return rigidbody_ ? rigidbody_->GetGameObject() : collider_->GetGameObject();
}


Transform *ou::Collision::GetTransform() const {
	return rigidbody_ ? rigidbody_->GetTransform() : collider_->GetTransform();
}


int ou::Collision::GetContactCount() const {
	return static_cast<int>(contacts_.size());
}


const ContactPoint &ou::Collision::GetContactPoint(int index) const {
	return contacts_.at(index);
}


const std::vector<ContactPoint> &ou::Collision::GetContacts() const {
	return contacts_;
}
