#pragma once

#include <cstdint>
#include <string_view>


namespace ou {

// 前方宣言
class Scene;


enum class NewSceneSetup {
	EmptyScene,
	DefaultGameObjects,
};

class EditorSceneManager final {
public: // 静的メンバ関数

	EditorSceneManager() = delete;

	static Scene *NewScene(NewSceneSetup setup);

	static Scene *OpenScene(std::string_view scenePath);

	static bool SaveScene(Scene *scene, std::string_view dstScenePath = {});

	static bool MarkSceneDirty(Scene *scene);

};

} // namespace ou
