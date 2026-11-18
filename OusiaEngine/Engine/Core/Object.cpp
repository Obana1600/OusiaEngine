#include "Object.h"
#include "Diagnostics/Log.h"
#include "Math/Matrix4x4.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"
#include "Scene/SceneManager.h"
#include "Serialize/SceneSerializer.h"

#include <functional>

using namespace ou;


namespace {

GameObject *InstantiateInto(GameObject *original, Transform *parent, const std::function<void(GameObject &)> &place) {
	if (!original || original->IsDestroyed()) {
		Log::Error("Instantiate に null か破棄された GameObject が渡されました");
		return nullptr;
	}

	auto *scene = parent ? parent->GetGameObject()->GetScene() : SceneManager::GetActiveScene();
	if (!scene) {
		Log::Error("シーンがないため Instantiate できません");
		return nullptr;
	}
	return SceneSerializer::Instantiate(*original, *scene, place);
}

void PlaceUnder(GameObject &clone, Transform *parent, const Vector3 &position, const Quaternion &rotation) {
	auto *transform = clone.GetTransform();
	if (parent) {
		transform->SetLocalPosition(parent->GetLocalToWorldMatrix().Inverse().MultiplyPoint3x4(position));
		transform->SetLocalRotation(Quaternion::Inverse(parent->GetWorldRotation()) * rotation);
	} else {
		transform->SetLocalPosition(position);
		transform->SetLocalRotation(rotation);
	}
	transform->SetParent(parent);
}

} // namespace



GameObject *ou::Object::Instantiate(GameObject *original) {
	return InstantiateInto(original, nullptr, [original](GameObject &clone) {
		PlaceUnder(clone, nullptr, original->GetTransform()->GetWorldPosition(), original->GetTransform()->GetWorldRotation());
	});
}


GameObject *ou::Object::Instantiate(GameObject *original, Transform *parent, bool instantiateInWorldSpace) {
	return InstantiateInto(original, parent, [original, parent, instantiateInWorldSpace](GameObject &clone) {
		if (instantiateInWorldSpace) {
			PlaceUnder(clone, parent, original->GetTransform()->GetWorldPosition(), original->GetTransform()->GetWorldRotation());
		} else {
			clone.GetTransform()->SetParent(parent);
		}
	});
}


GameObject *ou::Object::Instantiate(GameObject *original, const Vector3 &position, const Quaternion &rotation, Transform *parent) {
	return InstantiateInto(original, parent, [&position, &rotation, parent](GameObject &clone) {
		PlaceUnder(clone, parent, position, rotation);
	});
}


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
