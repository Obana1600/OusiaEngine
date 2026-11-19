#pragma once

#include "Core/JsonFile.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
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
	// .scene には version だけを書き、GameObject は 1 個ずつ隣の「<名前>.objects/」に書く（複数人で同じシーンを触っても衝突しないように）
	static bool Save(Scene &scene, const std::filesystem::path &path);

	static bool FromJson(Scene &scene, const Json &root);
	static bool Load(Scene &scene, const std::filesystem::path &path);

	// .scene と「<名前>.objects/」を読み、ToJson(scene) と同じ 1 つの json にまとめる
	// .scene に gameObjects があれば（書き出したもの・分ける前の形）、それをそのまま使う
	static std::optional<Json> LoadJson(const std::filesystem::path &path);

	static std::filesystem::path ToObjectsFolder(const std::filesystem::path &scenePath);
	static bool IsObjectsFolder(const std::filesystem::path &folder);

	static bool ValidateRoot(const Json &root);

	static SceneObjects CollectObjects(Scene &scene);

	static GameObject *Instantiate(GameObject &original, Scene &scene, const std::function<void(GameObject &)> &place);

	// ToJson(root, true) で書いた json から、新しい GameObject を作る（エディタの複製・ペースト）
	// 中どうしの参照は新しいものどうしに、外への参照は scene の今のものにつなぐ（見つからなければ null）
	// 元と fileId が重ならないように、作ったものの fileId は 0 にする（保存するときに振り直す）
	// place で親と並びを決めてから、アクティブかどうかを決める
	static GameObject *InstantiateFromJson(const Json &json, Scene &scene, const std::function<void(GameObject &)> &place);
	static std::vector<GameObject *> CreateGameObjects(Scene &scene, const Json &root, const std::unordered_map<uint64_t, Object *> &outside, bool awake);

	// Undo で戻す。シーンは作り直さず、今ある GameObject・Component に json を読み直させるので、ほかからの参照や選択はそのまま残る
	// json がないものは消し、シーンにないものは作る。並びは、渡したものの siblingIndex を小さい順に当てる
	struct RestoringGameObject {
		uint64_t id = 0;
		const Json *json = nullptr; // ToJson(scene) の gameObjects の要素
		int siblingIndex = 0;
	};
	static void RestoreGameObjects(Scene &scene, const std::vector<RestoringGameObject> &targets);


private: // 静的メンバ関数

	// json の components にそろえる（足したものは値を読まずに作るだけ。消すものは Destroy の印を付けるだけ）
	static void MatchComponents(GameObject &gameObject, const Json &gameObjectJson, std::unordered_map<uint64_t, Object *> &objectsById);

	static Json WriteGameObjects(const std::vector<GameObject *> &gameObjects, const std::unordered_set<const Object *> &savedObjects);
};

} // namespace ou
