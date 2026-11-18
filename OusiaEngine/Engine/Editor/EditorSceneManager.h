#pragma once

#include <string>
#include <string_view>


namespace ou {

// 前方宣言
class Scene;


enum class NewSceneSetup {
	EmptyScene,
	DefaultGameObjects,
};

class EditorSceneManager final {
	friend class Engine;
	friend class Undo;

public: // 静的メンバ関数

	EditorSceneManager() = delete;

	static Scene *NewScene(NewSceneSetup setup);

	static Scene *OpenScene(std::string_view scenePath);

	static bool SaveScene(Scene *scene, std::string_view dstScenePath = {});

	static bool MarkSceneDirty(Scene *scene);

	// AssetDatabase::MoveAsset のあとに呼ぶ。開いているシーン・Build Settings・次に開くシーンのパスを新しくする
	static void NotifyAssetMoved(const std::string &oldPath, const std::string &newPath);


private: // 静的メンバ関数

	static void OpenLastScene();

	// Undo で保存した状態に戻ったときに * を消すため
	static void SetDirtyFlag(Scene *scene, bool isDirty);

	static void RememberLastScene(std::string_view scenePath);
	static std::string LoadLastScenePath();
};

} // namespace ou
