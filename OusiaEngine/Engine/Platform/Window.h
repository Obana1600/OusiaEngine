#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>


namespace ou {

struct WindowDesc {
	std::string title;
	uint32_t width = 1280;
	uint32_t height = 720;
};


class Window {
public: // 静的ファクトリ

	static std::unique_ptr<Window> Create(const WindowDesc &desc);


public: // メッセージエイリアス

	using MessageHandler = std::function<std::optional<LRESULT>(HWND, UINT, WPARAM, LPARAM)>;
	using MessageHandlerId = uint32_t;


public: // メンバ関数

	~Window();

	void PumpMessages();

	MessageHandlerId AddMessageHandler(MessageHandler handler);
	void RemoveMessageHandler(MessageHandlerId id);

	HWND Hwnd() const;
	uint32_t ClientWidth() const;
	uint32_t ClientHeight() const;
	bool IsCloseRequested() const;
	bool IsMinimized() const;


private: // メッセージハンドラーの登録情報

	struct HandlerEntry {
		MessageHandler handler;
		MessageHandlerId id;
	};


private: // メンバ変数

	HWND hwnd_{};

	uint32_t clientWidth_ = 0;
	uint32_t clientHeight_ = 0;

	std::vector<HandlerEntry> handlers_;
	MessageHandlerId nextHandlerId_ = 1;

	bool closeRequested_ = false;
	bool minimized_ = false;
	bool resized_ = false;
	bool inSizeMove_ = false;


private: // プロシージャ

	static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


private: // コンストラクタ

	Window() = default;

	Window(const Window &) = delete;
	Window &operator=(const Window &) = delete;
	Window(Window &&) = delete;
	Window &operator=(Window &&) = delete;
};

} // namespace ou