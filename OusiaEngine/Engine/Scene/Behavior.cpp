#include "Behavior.h"
#include "Scene/GameObject.h"
#include "Serialize/Serializer.h"

using namespace ou;


void ou::Behavior::SetEnabled(bool value) {
	if (isEnabled_ == value) { return; }

	isEnabled_ = value;
	RefreshActiveAndEnabled();
}


bool ou::Behavior::IsEnabled() const {
	return isEnabled_;
}


bool ou::Behavior::IsActiveAndEnabled() const {
	return isEnabled_ && GetGameObject()->IsActiveInHierarchy();
}


void ou::Behavior::Serialize(Serializer &serializer) {
	serializer.Field("enabled", isEnabled_);
}


void ou::Behavior::AwakeFromLoad() {
	Component::AwakeFromLoad();

	// OnEnable を呼んだかどうか（wasActiveAndEnabled_）と食い違えば、OnEnable / OnDisable を呼ぶ
	RefreshActiveAndEnabled();
}


bool ou::Behavior::IsEnabledSelf() const {
	return isEnabled_;
}


void ou::Behavior::RefreshActiveAndEnabled() {
	const auto isActiveAndEnabled = IsActiveAndEnabled();
	if (isActiveAndEnabled == wasActiveAndEnabled_) { return; }

	wasActiveAndEnabled_ = isActiveAndEnabled;
	if (isActiveAndEnabled) {
		OnEnable();
	} else {
		OnDisable();
	}
}


void ou::Behavior::DisableForDestroy() {
	if (!wasActiveAndEnabled_) { return; }

	wasActiveAndEnabled_ = false;
	OnDisable();
}
