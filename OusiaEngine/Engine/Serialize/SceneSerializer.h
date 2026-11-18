#pragma once

#include "Core/JsonFile.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace ou {

// 前方宣言
class Scene;
class Object;
class GameObject;

// シーンの中で参照できるもの（JsonWriter と JsonReader に渡す）
struct SceneObjects {
	std::unordered_set<const Object *> savedObjects;	// 書くときに参照してよいもの
	std::unordered_map<uint64_t, Object *> objectsById; // 読むときに fileId から引く
};

class SceneSerializer final {
public: // 静的メンバ関数

	static Json ToJson(Scene &scene);
	static Json ToJson(GameObject &root, bool keepOutsideReferences);
	static bool Save(Scene &scene, const std::filesystem::path &path);

	static bool FromJson(Scene &scene, const Json &root);
	static bool Load(Scene &scene, const std::filesystem::path &path);

	static bool ValidateRoot(const Json &root);

	static SceneObjects CollectObjects(Scene &scene);

	static GameObject *Instantiate(GameObject &original, Scene &scene, const std::function<void(GameObject &)> &place);

	// ToJson(root, true) で書いた json から、新しい GameObject を作る（エディタの複製・ペースト）
	// 中どうしの参照は新しいものどうしに、外への参照は scene の今のものにつなぐ（見つからなければ null）
	// 元と fileId が重ならないように、作ったものの fileId は 0 にする（保存するときに振り直す）
	// place で親と並びを決めてから、アクティブかどうかを決める
	static GameObject *InstantiateFromJson(const Json &json, Scene &scene, const std::function<void(GameObject &)> &place);
	static std::vector<GameObject *> CreateGameObjects(Scene &scene, const Json &root, const std::unordered_map<uint64_t, Object *> &outside, bool awake);


private: // 静的メンバ関数

	static Json WriteGameObjects(const std::vector<GameObject *> &gameObjects, const std::unordered_set<const Object *> &savedObjects);
};

} // namespace ou
