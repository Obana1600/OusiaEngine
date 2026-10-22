#include "Behaviour.h"
#include "Scene/GameObject.h"

using namespace ou;


void ou::Behaviour::SetEnabled(bool value) {
	if (isEnabled_ == value) { return; }

	isEnabled_ = value;
	RefreshActiveAndEnabled();
}


bool ou::Behaviour::IsEnabled() const {
	return isEnabled_;
}


bool ou::Behaviour::IsActiveAndEnabled() const {
	return isEnabled_ && GetGameObject()->IsActiveInHierarchy();
}


bool ou::Behaviour::IsEnabledSelf() const {
	return isEnabled_;
}


void ou::Behaviour::RefreshActiveAndEnabled() {
	const auto isActiveAndEnabled = IsActiveAndEnabled();
	if (isActiveAndEnabled == wasActiveAndEnabled_) { return; }

	wasActiveAndEnabled_ = isActiveAndEnabled;
	if (isActiveAndEnabled) {
		OnEnable();
	} else {
		OnDisable();
	}
}


void ou::Behaviour::DisableForDestroy() {
	if (!wasActiveAndEnabled_) { return; }

	wasActiveAndEnabled_ = false;
	OnDisable();
}
