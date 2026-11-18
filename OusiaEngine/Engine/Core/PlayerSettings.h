#pragma once

#include "Platform/Window.h"

#include <cstdint>
#include <string>
#include <string_view>


namespace ou {

class PlayerSettings final {
	friend class Engine;

public: // 静的メンバ関数

	PlayerSettings() = delete;

	static void SetProductName(std::string_view productName);
	static const std::string &GetProductName();

	// Unity の Resolution and Presentation。書き出した Player の起動のしかた
	static void SetFullScreenMode(FullScreenMode mode);
	static FullScreenMode GetFullScreenMode();

	// Windowed で起動するときのクライアント領域の大きさ
	static void SetDefaultScreenSize(uint32_t width, uint32_t height);
	static uint32_t GetDefaultScreenWidth();
	static uint32_t GetDefaultScreenHeight();


private: // 静的メンバ変数

	static inline std::string sProductName_;
	static inline FullScreenMode sFullScreenMode_ = FullScreenMode::FullScreenWindow;
	static inline uint32_t sDefaultScreenWidth_ = 1280;
	static inline uint32_t sDefaultScreenHeight_ = 720;


private: // 静的メンバ関数

	static void Load(std::string_view fallbackProductName);
};

} // namespace ou
