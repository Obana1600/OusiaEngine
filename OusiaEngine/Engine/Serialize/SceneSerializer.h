#pragma once

#include "Core/JsonFile.h"

#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>


namespace ou {

// 前方宣言
class Object;
class Scene;

// シーンの中で参照できるもの（JsonWriter と JsonReader に渡す）
struct SceneObjects {
	std::unordered_set<const Object *> savedObjects;      // 書くときに参照してよいもの
	std::unordered_map<uint64_t, Object *> objectsById;  // 読むときに fileId から引く
};

class SceneSerializer final {
public: // 静的メンバ関数

	static Json ToJson(Scene &scene);
	static bool Save(Scene &scene, const std::filesystem::path &path);

	static bool FromJson(Scene &scene, const Json &root);
	static bool Load(Scene &scene, const std::filesystem::path &path);

	static bool ValidateRoot(const Json &root);

	// 保存されるもの（GameObject と登録済みの Component）に fileId を振って集める
	// 保存するときと同じ id になる（まだないものにだけ新しく振る）
	static SceneObjects CollectObjects(Scene &scene);
};

} // namespace ou
