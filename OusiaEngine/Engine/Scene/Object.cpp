#include "Object.h"

using namespace ou;


void ou::Object::Destroy(Object *obj) {
	if (obj) { obj->isDestroyed_ = true; }
}


ou::Object::Object()
	: instanceId_(sNextInstanceId_++) {}


uint32_t ou::Object::GetInstanceId() const {
	return instanceId_;
}


bool ou::Object::IsDestroyed() const {
	return isDestroyed_;
}

void ou::Object::SetName(std::string_view name) {
	name_ = name;
}


const std::string &ou::Object::GetName() const {
	return name_;
}
