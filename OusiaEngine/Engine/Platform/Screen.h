#pragma once

#include "Platform/Window.h"

#include <cstdint>


namespace ou {

class Screen final {
	friend class Engine;

public: // 静的メンバ関数

	Screen() = delete;

	static uint32_t GetWidth();
	static uint32_t GetHeight();

	static void SetFullScreenMode(FullScreenMode mode);
	static FullScreenMode GetFullScreenMode();

	static void SetFullScreen(bool fullScreen);
	static bool IsFullScreen();

	static void SetResolution(uint32_t width, uint32_t height, FullScreenMode mode);


private: // 静的メンバ変数

	static inline uint32_t sWidth_ = 0;
	static inline uint32_t sHeight_ = 0;
	static inline Window *sWindow_ = nullptr;


private: // 静的メンバ関数

	static void SetSize(uint32_t width, uint32_t height);
};

} // namespace ou
