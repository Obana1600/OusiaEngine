#include "Object.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"
#include "Scene/SceneManager.h"

using namespace ou;


void ou::Object::Destroy(Object *obj) {
	if (obj) { obj->isDestroyed_ = true; }
}


void ou::Object::DontDestroyOnLoad(Object *obj) {
	auto *gameObject = dynamic_cast<GameObject *>(obj);
	if (auto *component = dynamic_cast<Component *>(obj)) {
		gameObject = component->GetGameObject();
	}
	if (!gameObject) { return; }

	SceneManager::MoveGameObjectToScene(gameObject, SceneManager::GetDontDestroyOnLoadScene());
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
