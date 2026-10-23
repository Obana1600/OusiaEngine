#include "RawInput.h"
#include "Diagnostics/Fatal.h"
#include "Diagnostics/Log.h"

#include <iterator>

using namespace ou;
using namespace ou::Input;


namespace {

// HID の Generic Desktop のページと、その中のマウス / キーボード
constexpr USHORT kUsagePageGeneric = 0x01;
constexpr USHORT kUsageMouse = 0x02;
constexpr USHORT kUsageKeyboard = 0x06;

// マウスの各ボタンの押下・解放フラグ
struct ButtonFlags {
	USHORT down;
	USHORT up;
};

constexpr ButtonFlags kButtonFlags[RawInput::kMouseButtonCount] = {
	{RI_MOUSE_BUTTON_1_DOWN, RI_MOUSE_BUTTON_1_UP},
	{RI_MOUSE_BUTTON_2_DOWN, RI_MOUSE_BUTTON_2_UP},
	{RI_MOUSE_BUTTON_3_DOWN, RI_MOUSE_BUTTON_3_UP},
	{RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_4_UP},
	{RI_MOUSE_BUTTON_5_DOWN, RI_MOUSE_BUTTON_5_UP},
};

// グローバルなインスタンス
RawInput::KeyboardState sKeyboard;
RawInput::MouseState sMouse;


void ParseKeyboard(const RAWKEYBOARD &keyboard) {
	// キー番号の取得
	auto vk = keyboard.VKey;
	if (vk == 0 || vk >= 255) { return; }

	// 左右の修飾キーを区別する
	const auto isExtended = (keyboard.Flags & RI_KEY_E0) != 0;

	switch (vk) {
		case VK_SHIFT:
			vk = static_cast<USHORT>(MapVirtualKeyW(keyboard.MakeCode, MAPVK_VSC_TO_VK_EX));
			break;

		case VK_CONTROL:
			vk = isExtended ? VK_RCONTROL : VK_LCONTROL;
			break;

		case VK_MENU: // Alt キー
			vk = isExtended ? VK_RMENU : VK_LMENU;
			break;
	}

	sKeyboard.keys[vk] = (keyboard.Flags & RI_KEY_BREAK) == 0;
}

void ParseMouse(const RAWMOUSE &mouse) {
	// 相対座標の更新
	if ((mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0) {
		sMouse.deltaX += mouse.lLastX;
		sMouse.deltaY += mouse.lLastY;
	}

	// 各ボタンの押下状態の更新
	for (size_t i = 0; i < RawInput::kMouseButtonCount; i++) {
		if (mouse.usButtonFlags & kButtonFlags[i].down) { sMouse.buttons[i] = true; }
		if (mouse.usButtonFlags & kButtonFlags[i].up) { sMouse.buttons[i] = false; }
	}

	// ホイールの更新
	const auto wheel = static_cast<SHORT>(mouse.usButtonData) / static_cast<float>(WHEEL_DELTA);
	if (mouse.usButtonFlags & RI_MOUSE_WHEEL) { sMouse.wheel += wheel; }
	if (mouse.usButtonFlags & RI_MOUSE_HWHEEL) { sMouse.horizontalWheel += wheel; }
}

} // namespace



bool ou::Input::RawInput::Initialize(HWND hwnd) {
	// 登録するデバイス
	const RAWINPUTDEVICE devices[] = {
		{kUsagePageGeneric, kUsageMouse, 0, hwnd},
		{kUsagePageGeneric, kUsageKeyboard, 0, hwnd},
	};

	// Windows に渡す
	if (!RegisterRawInputDevices(devices, static_cast<UINT>(std::size(devices)), sizeof(RAWINPUTDEVICE))) {
		Log::Error("Raw Input の登録に失敗しました: {}", DescribeHResult(HRESULT_FROM_WIN32(GetLastError())));
		return false;
	}

	return true;
}


void ou::Input::RawInput::Finalize() {
	// 登録を解除するデバイス
	const RAWINPUTDEVICE devices[] = {
		{kUsagePageGeneric, kUsageMouse, RIDEV_REMOVE, nullptr},
		{kUsagePageGeneric, kUsageKeyboard, RIDEV_REMOVE, nullptr},
	};
	RegisterRawInputDevices(devices, static_cast<UINT>(std::size(devices)), sizeof(RAWINPUTDEVICE));

	sKeyboard = {};
	sMouse = {};
}


void ou::Input::RawInput::ParseMessage(LPARAM lparam) {
	RAWINPUT raw{};
	UINT size = sizeof(raw);

	// 入力データを取得する
	const auto result = GetRawInputData(
		reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER));
	if (result == static_cast<UINT>(-1)) { return; }

	// 入力状態の更新
	switch (raw.header.dwType) {
		case RIM_TYPEKEYBOARD:
			ParseKeyboard(raw.data.keyboard);
			break;

		case RIM_TYPEMOUSE:
			ParseMouse(raw.data.mouse);
			break;
	}
}


void ou::Input::RawInput::ReleaseAll() {
	sKeyboard = {};
	sMouse = {};
}


void ou::Input::RawInput::ClearMouseDelta() {
	sMouse.deltaX = 0;
	sMouse.deltaY = 0;
	sMouse.wheel = 0.0f;
	sMouse.horizontalWheel = 0.0f;
}


const RawInput::KeyboardState &ou::Input::RawInput::GetKeyboardState() {
	return sKeyboard;
}


const RawInput::MouseState &ou::Input::RawInput::GetMouseState() {
	return sMouse;
}
