#pragma once

#include "Editor/EditorWindow.h"
#include "Editor/ItemClick.h"

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
		Shader,
		Material,
		PhysicsMaterial,
		VolumeProfile,
	};


public: // メンバ関数

	ProjectWindow();


protected: // メンバ関数

	void OnGui() override;


private: // メンバ変数

	std::string selectedFolder_ = "Assets";
	bool revealSelectedFolder_ = false;

	// Create の名前を聞くポップアップ
	CreateKind creatingKind_ = CreateKind::Script;
	std::string creatingFolder_;
	std::string creatingName_;
	std::string creatingShader_; // Material で使うシェーダのアセットのパス
	bool opensCreatePopup_ = false;

	// Delete の確認のポップアップ
	std::string deletingPath_;
	bool opensDeletePopup_ = false;

	// 名前を変えているもの（空なら変えていない）。拡張子は変えないので、欄には名前だけを出す
	std::string renamingPath_;
	std::string renamingName_;
	std::string renamingExtension_;
	bool focusesRename_ = false;
	int renameInactiveFrames_ = 0;
	// 左のフォルダツリーで変えているか（右の一覧と同じフォルダが両方に出るため）
	bool isRenamingInTree_ = false;
	SlowClick<std::string> slowClick_;
	SlowClick<std::string> treeSlowClick_;
	SelectOnRelease selectOnRelease_;
	SelectOnRelease treeSelectOnRelease_;


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

	// Unity と同じく、その場で名前の欄にする（ファイルは .meta も一緒に名前を変えるので、参照は切れない）
	bool CanRename(const std::string &assetPath) const;
	void BeginRename(const std::string &assetPath, bool inTree = false);
	// 行を名前の欄にして描く（行の代わりに呼ぶ）
	void DrawRenameRow(const std::string &assetPath);
	// 直前の行の上に名前の欄を重ねる（labelX は名前を出す位置）
	void DrawRenameField(const std::string &assetPath, float labelX);
	// 今の名前で変えられないなら、その理由
	std::string FindRenameProblem() const;
	void EndRename(bool commits);

	// フォルダツリーの行の右クリック
	void DrawFolderTreeMenu(const std::string &assetPath);

	// ドラッグで、ファイルとフォルダを folder の中へ動かす（同じ名前のものがあれば、動かさずにエラー）
	bool CanMove(const std::string &source, const std::string &folder) const;
	// folder の行を、動かす先・プレハブを作る先として受ける（BeginDragDropTarget の中で呼ぶ）
	void AcceptDropOnFolder(const std::string &folder);
	void MoveAssetInto(const std::string &source, const std::string &folder);
	// 動かした・名前を変えたあとに、開いているシーン・選んでいるもの・開いているフォルダを新しいパスにする
	void AfterAssetMoved(const std::string &oldPath, const std::string &newPath);

	// Unity と同じく、確かめてから消す（ファイルと .meta を Windows のごみ箱へ移す）
	void BeginDelete(const std::string &assetPath);
	void DrawDeletePopup();

	// Hierarchy から落とした GameObject を .prefab にする（target はフォルダか .prefab）
	void AcceptGameObjectDrop(const std::string &target);
	void SaveDroppedGameObject(const std::string &target);
};

} // namespace ou
