#pragma once

#include <filesystem>

#include <nlohmann/json_fwd.hpp>


namespace ou {

// 前方宣言
class Scene;

class SceneSerializer final {
public: // 静的メンバ関数

	static nlohmann::ordered_json ToJson(Scene &scene);
	static bool Save(Scene &scene, const std::filesystem::path &path);

	static bool FromJson(Scene &scene, const nlohmann::ordered_json &root);
	static bool Load(Scene &scene, const std::filesystem::path &path);
};

} // namespace ou
