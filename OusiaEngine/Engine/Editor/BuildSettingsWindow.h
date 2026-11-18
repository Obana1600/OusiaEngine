#pragma once

#include "Editor/EditorBuildSettings.h"
#include "Editor/EditorWindow.h"

#include <string>
#include <vector>


namespace ou {

// Unity の File > Build Settings の Scenes In Build
class BuildSettingsWindow final: public EditorWindow {
public: // メンバ関数

	BuildSettingsWindow();


protected: // メンバ関数

	void OnGui() override;


private: // メンバ変数

	std::vector<EditorBuildSettingsScene> scenes_;
	bool isLoaded_ = false;
	int selected_ = -1;


private: // メンバ関数

	void DrawSceneList();
	void AddScene(const std::string &scenePath);
	void Save();
	void DrawBuildArea();
};

} // namespace ou
