#pragma once

#include <string>


namespace ou {

class EditorMainWindow final {
	friend class Engine;
	friend class ProjectWindow;

public: // 静的メンバ関数

	EditorMainWindow() = delete;


private: // 静的メンバ関数

	static void Draw();

	static void RequestOpenScene(const std::string &scenePath);
};

} // namespace ou
