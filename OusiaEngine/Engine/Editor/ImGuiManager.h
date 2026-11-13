#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Platform/Window.h"

#include <string>


namespace ou {

class ImGuiManager {
public: // メンバ関数

	explicit ImGuiManager(Window *window);
	ImGuiManager(const ImGuiManager &) = delete;
	ImGuiManager &operator=(const ImGuiManager &) = delete;

	~ImGuiManager();

	void NewFrame();
	void EndFrame();

	void Render(Graphics::CommandList cmd);


private: // メンバ変数

	Window *window_ = nullptr;
	Window::MessageHandlerId messageHandlerId_ = 0;

	// TODO: フォントを自由に変更できるようにしたい
	const std::string kUseJpFontName = "NotoSansJP-Bold.ttf";
	const float kFontSize = 18.0f;
};

} // namespace ou
