#include "Component.h"

using namespace ou;


GameObject *ou::Component::GetGameObject() const {
	return gameObject_;
}


Transform *ou::Component::GetTransform() const {
	return transform_;
}


bool ou::Component::IsEnabledSelf() const {
	return true;
}
