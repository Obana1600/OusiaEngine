#pragma once

#include "Editor/EditorWindow.h"

#include <cstdint>
#include <filesystem>
#include <string>


namespace ou {

class ProjectWindow final: public EditorWindow {
private: // インナークラス

	// 右クリックの Create で作るもの
	enum class CreateKind : uint8_t {
		Folder,
		Script,
	};


public: // メンバ関数

	ProjectWindow();


protected: // メンバ関数

	void OnGui() override;


private: // メンバ変数

	std::string selectedFolder_ = "Assets";
	std::string selectedItem_;
	bool revealSelectedFolder_ = false;

	// Create の名前を聞くポップアップ
	CreateKind creatingKind_ = CreateKind::Script;
	std::string creatingFolder_;
	std::string creatingName_;
	bool opensCreatePopup_ = false;


private: // メンバ関数

	void DrawFolderTree(const std::string &assetPath, const std::filesystem::path &fullPath);

	void DrawContents();
	void DrawFolderItem(const std::string &assetPath, const std::string &name);
	void DrawFileItem(const std::string &assetPath, const std::filesystem::path &fullPath);
	void DrawSubAssets(const std::string &assetPath);

	void HandleItem(const std::string &assetPath, const std::string &subAsset);

	void DrawItemMenu(const std::string &assetPath);
	void DrawCreateMenu();
	void DrawCreatePopup();
	void BeginCreate(CreateKind kind);
	void OpenAsset(const std::string &assetPath);
};

} // namespace ou
