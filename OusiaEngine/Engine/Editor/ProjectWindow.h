#pragma once

#include "Editor/EditorWindow.h"

#include <filesystem>
#include <string>


namespace ou {

class ProjectWindow final: public EditorWindow {
public: // メンバ関数

	ProjectWindow();


protected: // メンバ関数

	void OnGui() override;


private: // メンバ変数

	std::string selectedFolder_ = "Assets";
	std::string selectedItem_;


private: // メンバ関数

	void DrawFolderTree(const std::string &assetPath, const std::filesystem::path &fullPath);

	void DrawContents();
	void DrawFolderItem(const std::string &assetPath, const std::string &name);
	void DrawFileItem(const std::string &assetPath, const std::filesystem::path &fullPath);
	void DrawSubAssets(const std::string &assetPath);

	void HandleItem(const std::string &assetPath, const std::string &subAsset);
};

} // namespace ou
