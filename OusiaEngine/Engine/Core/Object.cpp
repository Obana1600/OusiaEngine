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


Object *ou::Object::FindObjectFromInstanceId(uint32_t instanceId) {
	const auto it = sObjects_.find(instanceId);
	return it != sObjects_.end() ? it->second : nullptr;
}


ou::Object::Object()
	: instanceId_(sNextInstanceId_++) {
	sObjects_.emplace(instanceId_, this);
}


ou::Object::~Object() {
	sObjects_.erase(instanceId_);
}


uint32_t ou::Object::GetInstanceId() const {
	return instanceId_;
}


bool ou::Object::IsDestroyed() const {
	return isDestroyed_;
}


void ou::Object::SetFileId(uint64_t fileId) {
	fileId_ = fileId;
}


uint64_t ou::Object::GetFileId() const {
	return fileId_;
}


void ou::Object::SetName(std::string_view name) {
	name_ = name;
}


const std::string &ou::Object::GetName() const {
	return name_;
}
