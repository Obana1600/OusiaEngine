#include "SceneSerializer.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Serialize/JsonReader.h"
#include "Serialize/JsonWriter.h"
#include "Serialize/TypeRegistry.h"

#include <random>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace ou;


namespace {

// TODO: version管理をどうするか決める
constexpr int kVersion = 1;

// GameObject / Component の予約のキー
constexpr const char *kTypeKey = "$type";
constexpr const char *kIdKey = "$id";


void CollectHierarchy(GameObject *gameObject, const Scene *scene, std::vector<GameObject *> &out) {
	if (gameObject->IsDestroyed() || gameObject->GetScene() != scene) { return; }

	out.push_back(gameObject);
	for (auto *child : gameObject->GetTransform()->GetChildren()) {
		CollectHierarchy(child->GetGameObject(), scene, out);
	}
}

void AssignFileId(Object *object, std::unordered_set<uint64_t> &usedIds) {
	static std::mt19937_64 engine{std::random_device{}()};
	constexpr uint64_t kMask = (1ull << 53) - 1;

	auto id = object->GetFileId();
	while (id == 0 || usedIds.contains(id)) {
		id = engine() & kMask;
	}

	object->SetFileId(id);
	usedIds.insert(id);
}

const Json *FindMember(const Json &object, const char *key) {
	const auto it = object.find(key);
	return it != object.end() ? &*it : nullptr;
}

void RegisterFileId(Object *object, const Json &json, std::unordered_map<uint64_t, Object *> &objectsById) {
	const auto *id = FindMember(json, kIdKey);
	if (!id || !id->is_number_unsigned() || id->get<uint64_t>() == 0) { return; }

	const auto [it, inserted] = objectsById.emplace(id->get<uint64_t>(), object);
	if (!inserted) {
		Log::Warning("id が重なっています (後ろのものは参照できません): {}", id->get<uint64_t>());
		return;
	}
	object->SetFileId(id->get<uint64_t>());
}

} // namespace



Json ou::SceneSerializer::ToJson(Scene &scene) {
	std::vector<GameObject *> gameObjects;
	for (auto *root : scene.GetRootGameObjects()) {
		CollectHierarchy(root, &scene, gameObjects);
	}

	const auto objects = CollectObjects(scene);

	auto root = Json::object();
	root["version"] = kVersion;
	root["gameObjects"] = WriteGameObjects(gameObjects, objects.savedObjects);
	return root;
}


Json ou::SceneSerializer::ToJson(GameObject &root, bool keepOutsideReferences) {
	auto *scene = root.GetScene();
	auto objects = CollectObjects(*scene);

	std::vector<GameObject *> gameObjects;
	CollectHierarchy(&root, scene, gameObjects);

	if (!keepOutsideReferences) {
		std::unordered_set<const Object *> inside;
		for (auto *gameObject : gameObjects) {
			inside.insert(gameObject);
			for (const auto &component : gameObject->components_) {
				if (objects.savedObjects.contains(component.get())) { inside.insert(component.get()); }
			}
		}
		objects.savedObjects = std::move(inside);
	}

	if (auto *parent = root.GetTransform()->GetParent()) {
		objects.savedObjects.insert(parent);
	}

	auto json = Json::object();
	json["version"] = kVersion;
	json["gameObjects"] = WriteGameObjects(gameObjects, objects.savedObjects);

	if (!json["gameObjects"].empty()) {
		const auto transformId = root.GetTransform()->GetFileId();
		for (auto &componentJson : json["gameObjects"][0]["components"]) {
			if (componentJson[kIdKey] == transformId) { componentJson["parent"] = nullptr; }
		}
	}
	return json;
}


bool ou::SceneSerializer::Save(Scene &scene, const std::filesystem::path &path) {
	return JsonFile::Save(path, ToJson(scene));
}


bool ou::SceneSerializer::FromJson(Scene &scene, const Json &root) {
	if (!ValidateRoot(root)) { return false; }

	CreateGameObjects(scene, root, {}, true);
	return true;
}


bool ou::SceneSerializer::Load(Scene &scene, const std::filesystem::path &path) {
	const auto root = JsonFile::Load(path);
	if (!root) { return false; }

	return FromJson(scene, *root);
}


bool ou::SceneSerializer::ValidateRoot(const Json &root) {
	if (!root.is_object()) {
		Log::Error("json の一番外側がオブジェクトではありません");
		return false;
	}

	const auto *version = FindMember(root, "version");
	if (!version || !version->is_number_integer() || version->get<int>() < 1 || version->get<int>() > kVersion) {
		Log::Error("対応していないシーンのバージョンです (対応しているのは {} まで)", kVersion);
		return false;
	}

	const auto *gameObjectsJson = FindMember(root, "gameObjects");
	if (!gameObjectsJson || !gameObjectsJson->is_array()) {
		Log::Error("シーンの json に \"gameObjects\" の配列がありません");
		return false;
	}

	return true;
}


SceneObjects ou::SceneSerializer::CollectObjects(Scene &scene) {
	std::vector<GameObject *> gameObjects;
	for (auto *root : scene.GetRootGameObjects()) {
		CollectHierarchy(root, &scene, gameObjects);
	}

	SceneObjects objects;
	std::unordered_set<uint64_t> usedIds;
	const auto add = [&](Object *object) {
		AssignFileId(object, usedIds);
		objects.savedObjects.insert(object);
		objects.objectsById.emplace(object->GetFileId(), object);
	};

	for (auto *gameObject : gameObjects) {
		add(gameObject);
		for (const auto &component : gameObject->components_) {
			const Object &object = *component;
			if (component->IsDestroyed() || !TypeRegistry::FindName(object)) { continue; }
			add(component.get());
		}
	}
	return objects;
}


GameObject *ou::SceneSerializer::Instantiate(GameObject &original, Scene &scene, const std::function<void(GameObject &)> &place) {
	const auto outside = CollectObjects(*original.GetScene()).objectsById;
	const auto json = ToJson(original, true);

	const auto created = CreateGameObjects(scene, json, outside, false);
	if (created.empty()) { return nullptr; }

	auto *clone = created.front();
	clone->SetName(original.GetName() + "(Clone)");

	place(*clone);
	clone->RefreshActiveInHierarchy();
	return clone;
}


std::vector<GameObject *> ou::SceneSerializer::CreateGameObjects(Scene &scene, const Json &root, const std::unordered_map<uint64_t, Object *> &outside, bool awake) {
	const auto *gameObjectsJson = FindMember(root, "gameObjects");
	std::vector<std::pair<GameObject *, const Json *>> loadedGameObjects;
	std::vector<std::pair<Component *, const Json *>> loadedComponents;
	std::unordered_map<uint64_t, Object *> objectsById;

	for (const auto &gameObjectJson : *gameObjectsJson) {
		if (!gameObjectJson.is_object()) {
			Log::Warning("\"gameObjects\" の要素がオブジェクトではありません");
			continue;
		}

		auto *gameObject = scene.CreateGameObject();
		gameObject->isActiveSelf_ = false;
		RegisterFileId(gameObject, gameObjectJson, objectsById);
		loadedGameObjects.emplace_back(gameObject, &gameObjectJson);

		const auto *componentsJson = FindMember(gameObjectJson, "components");
		if (!componentsJson || !componentsJson->is_array()) { continue; }

		bool hasTransform = false;
		for (const auto &componentJson : *componentsJson) {
			const auto *type = componentJson.is_object() ? FindMember(componentJson, kTypeKey) : nullptr;
			if (!type || !type->is_string()) {
				Log::Warning("Component に \"type\" がありません (無視します)");
				continue;
			}

			const auto typeName = type->get<std::string>();
			auto created = TypeRegistry::Create(typeName);
			if (!created) {
				Log::Warning("型が登録されてないため読み込めません: {}", typeName);
				continue;
			}

			Component *component = nullptr;
			if (dynamic_cast<Transform *>(created.get())) {
				if (hasTransform) {
					Log::Warning("Transform が 2 つあります: {}", typeName);
					continue;
				}
				hasTransform = true;

				const Component &createdRef = *created;
				if (typeid(createdRef) == typeid(Transform)) {
					component = gameObject->GetTransform();
				} else {
					component = created.get();
					gameObject->ReplaceTransform(std::unique_ptr<Transform>(static_cast<Transform *>(created.release())));
				}
			} else {
				component = gameObject->AttachComponent(std::move(created));
			}

			RegisterFileId(component, componentJson, objectsById);
			loadedComponents.emplace_back(component, &componentJson);
		}
	}

	for (const auto &[id, object] : outside) {
		objectsById.emplace(id, object);
	}

	for (const auto &[component, json] : loadedComponents) {
		JsonReader reader(*json, objectsById);
		component->Serialize(reader);
	}

	for (const auto &[component, json] : loadedComponents) {
		component->AwakeFromLoad();
	}

	for (const auto &[gameObject, json] : loadedGameObjects) {
		gameObject->isActiveSelf_ = true;

		JsonReader reader(*json, objectsById);
		gameObject->Serialize(reader);
		gameObject->AwakeFromLoad();
	}

	std::vector<GameObject *> created;
	created.reserve(loadedGameObjects.size());
	for (const auto &[gameObject, json] : loadedGameObjects) {
		created.push_back(gameObject);
	}

#ifdef WITH_EDITOR
	// Unity と同じく、エディタではシーンを読んだときにも OnValidate を呼ぶ（Instantiate やプレハブの読み込みでは呼ばない）
	if (awake) {
		for (const auto &[component, json] : loadedComponents) {
			component->OnValidate();
		}
	}
#endif

	if (awake) {
		for (auto *gameObject : created) {
			if (!gameObject->GetTransform()->GetParent()) {
				gameObject->RefreshActiveInHierarchy();
			}
		}
	}

	return created;
}


Json ou::SceneSerializer::WriteGameObjects(const std::vector<GameObject *> &gameObjects, const std::unordered_set<const Object *> &savedObjects) {
	auto gameObjectsJson = Json::array();
	for (auto *gameObject : gameObjects) {
		auto gameObjectJson = Json::object();
		gameObjectJson[kIdKey] = gameObject->GetFileId();
		JsonWriter gameObjectWriter(gameObjectJson, savedObjects);
		gameObject->Serialize(gameObjectWriter);

		auto componentsJson = Json::array();
		for (const auto &component : gameObject->components_) {
			if (component->IsDestroyed()) { continue; }

			const Object &object = *component;
			const auto *typeName = TypeRegistry::FindName(object);
			if (!typeName) {
				Log::Warning("型が登録されてないため保存できません: {} ({})",
							 typeid(object).name(), gameObject->GetName());
				continue;
			}

			auto componentJson = Json::object();
			componentJson[kTypeKey] = *typeName;
			componentJson[kIdKey] = component->GetFileId();
			JsonWriter componentWriter(componentJson, savedObjects);
			component->Serialize(componentWriter);

			componentsJson.push_back(std::move(componentJson));
		}

		gameObjectJson["components"] = std::move(componentsJson);
		gameObjectsJson.push_back(std::move(gameObjectJson));
	}
	return gameObjectsJson;
}
