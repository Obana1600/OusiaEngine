#pragma once

#include <Windows.h>
#include <array>
#include <cstdint>


namespace ou::Input::RawInput {

// Raw Input から送られるマウスのボタンの数
constexpr size_t kMouseButtonCount = 5;

struct KeyboardState {
	std::array<bool, 256> keys{};
};

struct MouseState {
	std::array<bool, kMouseButtonCount> buttons{};
	int32_t deltaX = 0;
	int32_t deltaY = 0;
	float wheel = 0.0f;
	float horizontalWheel = 0.0f;
};


bool Initialize(HWND hwnd);
void Finalize();

// WM_INPUT の LPARAM を渡す
void ParseMessage(LPARAM lparam);

void ReleaseAll();

void ClearMouseDelta();

const KeyboardState &GetKeyboardState();
const MouseState &GetMouseState();

} // namespace ou::Input::RawInput
