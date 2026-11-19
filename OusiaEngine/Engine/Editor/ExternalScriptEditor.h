#pragma once

#include <filesystem>
#include <string>
#include <vector>


namespace ou {

// Unity の Preferences > External Tools > External Script Editor
// .as を開くエディタを探して、開く。選んだエディタは EditorPrefs に覚える
// Unity にない名前なので、全部 private にして使う画面を friend にする
class ExternalScriptEditor final {
	friend class ConsoleWindow;
	friend class ProjectWindow;
	friend class PreferencesWindow;
	friend class ShaderEditor;

public: // 静的メンバ関数

	ExternalScriptEditor() = delete;


private: // インナークラス

	struct Installation {
		std::string name;
		std::filesystem::path path;
	};


private: // 静的メンバ関数

	// この PC にあるエディタ（VS Code・Visual Studio・メモ帳）
	static std::vector<Installation> FindInstallations();

	// 選んでいるエディタ。選んでいなければ、見つかった最初のもの
	static std::filesystem::path FindCurrentPath();
	static void SetCurrentPath(const std::filesystem::path &path);

	// line は 1 から。0 なら行を指定しない
	static bool OpenFile(const std::filesystem::path &file, int line = 0);
};

} // namespace ou
