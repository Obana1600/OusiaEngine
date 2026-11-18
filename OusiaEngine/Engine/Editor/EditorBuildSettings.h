#pragma once

#include "Core/Guid.h"

#include <string>
#include <vector>


namespace ou {

// Unity の EditorBuildSettingsScene
struct EditorBuildSettingsScene {
	std::string path;
	Guid guid;
	bool enabled = true;
};


// Unity の EditorBuildSettings
// ProjectSettings/EditorBuildSettings.json を読み書きする
class EditorBuildSettings final {
public: // 静的メンバ関数

	EditorBuildSettings() = delete;

	static std::vector<EditorBuildSettingsScene> GetScenes();
	static void SetScenes(const std::vector<EditorBuildSettingsScene> &scenes);
};

} // namespace ou
