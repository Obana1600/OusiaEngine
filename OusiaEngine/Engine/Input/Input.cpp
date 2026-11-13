#include "Input.h"
#include "Input/Windows/RawInput.h"
#include "Input/Windows/XInput.h"
#include "Platform/Window.h"

#include <optional>

using namespace ou;


namespace {

// ----- グローバルなインスタンス -----
// TODO: 静的メンバ変数にする

Window *sWindow = nullptr;
Window::MessageHandlerId sMessageHandlerId = 0;

RawInput::KeyboardState sKeyboard;
RawInput::KeyboardState sPrevKeyboard;
std::array<bool, RawInput::kMouseButtonCount> sMouseButtons{};
std::array<bool, RawInput::kMouseButtonCount> sPrevMouseButtons{};

Vector2 sMousePosition{};
Vector2 sMouseDelta{};
Vector2 sMouseScrollDelta{};

std::array<XInput::GamepadState, XInput::kMaxGamepadCount> sGamepads{};
std::array<XInput::GamepadState, XInput::kMaxGamepadCount> sPrevGamepads{};

bool sIsWindowActive = true; // ウィンドウがアクティブか

#ifdef WITH_EDITOR
Vector2 sGameViewOrigin{};
bool sGameViewHasFocus = false;
bool sGameViewIsHovered = false;
#endif


bool IsValidMouseButton(int button) {
	return 0 <= button && button < static_cast<int>(sMouseButtons.size());
}

const XInput::GamepadState *FindGamepad(int player) {
	if (player < 0 || player >= static_cast<int>(sGamepads.size())) { return nullptr; }
	return &sGamepads[player];
}

bool HasButton(const XInput::GamepadState &pad, GamepadButton button) {
	return (pad.buttons & static_cast<uint16_t>(button)) != 0;
}

} // namespace



bool ou::Input::GetKey(KeyCode key) {
	return sKeyboard.keys[static_cast<size_t>(key)];
}


bool ou::Input::GetKeyDown(KeyCode key) {
	const auto index = static_cast<size_t>(key);
	return sKeyboard.keys[index] && !sPrevKeyboard.keys[index];
}


bool ou::Input::GetKeyUp(KeyCode key) {
	const auto index = static_cast<size_t>(key);
	return !sKeyboard.keys[index] && sPrevKeyboard.keys[index];
}


bool ou::Input::GetMouseButton(int button) {
	if (!IsValidMouseButton(button)) { return false; }
	return sMouseButtons[button];
}


bool ou::Input::GetMouseButtonDown(int button) {
	if (!IsValidMouseButton(button)) { return false; }
	return sMouseButtons[button] && !sPrevMouseButtons[button];
}


bool ou::Input::GetMouseButtonUp(int button) {
	if (!IsValidMouseButton(button)) { return false; }
	return !sMouseButtons[button] && sPrevMouseButtons[button];
}


Vector2 ou::Input::GetMousePosition() {
	return sMousePosition;
}


Vector2 ou::Input::GetMouseDelta() {
	return sMouseDelta;
}


Vector2 ou::Input::GetMouseScrollDelta() {
	return sMouseScrollDelta;
}


bool ou::Input::IsGamepadConnected(int player) {
	const auto *pad = FindGamepad(player);
	return pad && pad->isConnected;
}


bool ou::Input::GetGamepadButton(GamepadButton button, int player) {
	const auto *pad = FindGamepad(player);
	return pad && HasButton(*pad, button);
}


bool ou::Input::GetGamepadButtonDown(GamepadButton button, int player) {
	const auto *pad = FindGamepad(player);
	return pad && HasButton(*pad, button) && !HasButton(sPrevGamepads[player], button);
}


bool ou::Input::GetGamepadButtonUp(GamepadButton button, int player) {
	const auto *pad = FindGamepad(player);
	return pad && !HasButton(*pad, button) && HasButton(sPrevGamepads[player], button);
}


Vector2 ou::Input::GetGamepadLeftStick(int player) {
	const auto *pad = FindGamepad(player);
	return pad ? pad->leftStick : Vector2::Zero;
}


Vector2 ou::Input::GetGamepadRightStick(int player) {
	const auto *pad = FindGamepad(player);
	return pad ? pad->rightStick : Vector2::Zero;
}


float ou::Input::GetGamepadLeftTrigger(int player) {
	const auto *pad = FindGamepad(player);
	return pad ? pad->leftTrigger : 0.0f;
}


float ou::Input::GetGamepadRightTrigger(int player) {
	const auto *pad = FindGamepad(player);
	return pad ? pad->rightTrigger : 0.0f;
}


void ou::Input::SetGamepadMotorSpeeds(float lowFrequency, float highFrequency, int player) {
	if (!FindGamepad(player)) { return; }
	XInput::SetMotorSpeeds(static_cast<size_t>(player), lowFrequency, highFrequency);
}


bool ou::Input::Initialize(Window *window) {
	if (!RawInput::Initialize(window->Hwnd())) { return false; }

	sWindow = window;
	sMessageHandlerId = window->AddMessageHandler(
		[](HWND, UINT msg, WPARAM wparam, LPARAM lparam) -> std::optional<LRESULT> {
			switch (msg) {
				case WM_INPUT:
					RawInput::ParseMessage(lparam);
					break;

				case WM_ACTIVATEAPP:
					sIsWindowActive = (wparam != FALSE);
					if (sIsWindowActive) {
						XInput::RequestScan();
					} else {
						RawInput::ReleaseAll();
						XInput::ReleaseAll();
					}
					break;

				case WM_DEVICECHANGE:
					XInput::RequestScan();
					break;
			}

			return std::nullopt;
		});

	return true;
}


void ou::Input::Finalize() {
	if (sWindow) {
		sWindow->RemoveMessageHandler(sMessageHandlerId);
		sWindow = nullptr;
	}

	RawInput::Finalize();
	XInput::Finalize();
}


void ou::Input::Update() {
	// キーボード
	sPrevKeyboard = sKeyboard;
	sKeyboard = RawInput::GetKeyboardState();

	// マウス
	sPrevMouseButtons = sMouseButtons;
	const auto &mouse = RawInput::GetMouseState();
	sMouseButtons = mouse.buttons;
	sMouseDelta = {static_cast<float>(mouse.deltaX), static_cast<float>(mouse.deltaY)};
	sMouseScrollDelta = {mouse.horizontalWheel, mouse.wheel};
	RawInput::ClearMouseDelta();
#ifdef WITH_EDITOR
	if (!sGameViewHasFocus) {
		sKeyboard = {};
		sMouseButtons = {};
		sMouseDelta = {};
		sMouseScrollDelta = {};
	} else if (!sGameViewIsHovered) {
		for (size_t i = 0; i < sMouseButtons.size(); i++) {
			sMouseButtons[i] = sMouseButtons[i] && sPrevMouseButtons[i];
		}
		sMouseScrollDelta = {};
	}
#endif

	// ウィンドウ上のカーソル座標を取得
	if (sWindow) {
		POINT point{};
		GetCursorPos(&point);
		ScreenToClient(sWindow->Hwnd(), &point);
		sMousePosition = {static_cast<float>(point.x), static_cast<float>(point.y)};
#ifdef WITH_EDITOR
		sMousePosition -= sGameViewOrigin;
#endif
	}

	// ゲームパッド
	sPrevGamepads = sGamepads;
	if (sIsWindowActive) { XInput::Update(); }
	for (size_t i = 0; i < sGamepads.size(); i++) {
		sGamepads[i] = XInput::GetState(i);
	}
}


#ifdef WITH_EDITOR
void ou::Input::SetGameView(const Vector2 &origin, bool hasFocus, bool isHovered) {
	sGameViewOrigin = origin;
	sGameViewHasFocus = hasFocus;
	sGameViewIsHovered = isHovered;
}
#endif
