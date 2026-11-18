#include "SerializedObject.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/Undo.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"
#include "Serialize/JsonReader.h"
#include "Serialize/JsonWriter.h"

#include <algorithm>

using namespace ou;


namespace {

Scene *FindScene(Object *object) {
	if (auto *gameObject = dynamic_cast<GameObject *>(object)) { return gameObject->GetScene(); }
	if (auto *component = dynamic_cast<Component *>(object)) { return component->GetGameObject()->GetScene(); }
	return nullptr;
}

std::string FindTopLevelKey(std::string_view path) {
	auto pointer = Json::json_pointer(std::string(path));
	while (!pointer.parent_pointer().empty()) {
		pointer = pointer.parent_pointer();
	}
	return pointer.back();
}

} // namespace



ou::SerializedObject::SerializedObject(Object *target)
	: target_(target) {
	Update();
}


Object *ou::SerializedObject::GetTargetObject() const {
	return target_;
}


void ou::SerializedObject::Update() {
	json_ = Json::object();
	schema_.clear();
	schemaIndices_.clear();
	modifiedKeys_.clear();
	if (!target_) { return; }

	auto *scene = FindScene(target_);
	sceneObjects_ = scene ? SceneSerializer::CollectObjects(*scene) : SceneObjects{};

	JsonWriter writer(json_, sceneObjects_.savedObjects);
	writer.SetSchemaOutput(&schema_);
	target_->Serialize(writer);

	for (size_t i = 0; i < schema_.size(); i++) {
		schemaIndices_.emplace(schema_[i].path, i);
	}
}


bool ou::SerializedObject::ApplyModifiedProperties() {
	if (!target_ || modifiedKeys_.empty()) { return false; }

	auto partial = Json::object();
	for (const auto &key : modifiedKeys_) {
		partial[key] = json_[key];
	}
	modifiedKeys_.clear();

	JsonReader reader(partial, sceneObjects_.objectsById);
	target_->Serialize(reader);
	target_->AwakeFromLoad();
	if (auto *component = dynamic_cast<Component *>(target_)) { component->OnValidate(); }

	if (auto *scene = FindScene(target_)) {
		Undo::SetCurrentGroupName("Modify " + target_->GetName());
		EditorSceneManager::MarkSceneDirty(scene);
	}
	return true;
}


bool ou::SerializedObject::HasModifiedProperties() const {
	return !modifiedKeys_.empty();
}


std::optional<SerializedProperty> ou::SerializedObject::FindProperty(std::string_view propertyPath) {
	auto path = "/" + std::string(propertyPath);
	if (!schemaIndices_.contains(path)) { return std::nullopt; }

	return SerializedProperty(this, std::move(path));
}


SerializedProperty ou::SerializedObject::GetIterator() {
	return SerializedProperty(this, std::string());
}


const PropertySchema *ou::SerializedObject::FindSchema(const std::string &path) const {
	const auto it = schemaIndices_.find(path);
	return it != schemaIndices_.end() ? &schema_[it->second] : nullptr;
}


void ou::SerializedObject::MarkModified(std::string_view path) {
	auto key = FindTopLevelKey(path);
	if (std::find(modifiedKeys_.begin(), modifiedKeys_.end(), key) == modifiedKeys_.end()) {
		modifiedKeys_.push_back(std::move(key));
	}
}
