#include "InputSystem.h"
#include "base/Window.h"

#include <cassert>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")


// public func ====================================================================================

InputSystem *InputSystem::GetInstance() {
	static InputSystem instance;
	return &instance;
}


void InputSystem::Initialize(HWND hwnd) {
	hwnd_ = hwnd;

	// DirectInputの初期化
	HRESULT hr = DirectInput8Create(
		GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8,
		reinterpret_cast<void **>(directInput_.ReleaseAndGetAddressOf()), nullptr
	);
	assert(SUCCEEDED(hr));


	// keuboard ----------------------------------------------------------

	// デバイスの生成
	hr = directInput_->CreateDevice(GUID_SysKeyboard, devKeyboard_.GetAddressOf(), NULL);
	assert(SUCCEEDED(hr));

	// 入力データ形式のセット
	hr = devKeyboard_->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(hr));

	// 排他制御レベルのセット
	hr = devKeyboard_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(hr));

	// -------------------------------------------------------------------


	// mouse -------------------------------------------------------------

	// マウスの生成
	hr = directInput_->CreateDevice(GUID_SysMouse, devMouse_.GetAddressOf(), NULL);
	assert(SUCCEEDED(hr));

	// 入力データ形式のセット
	hr = devMouse_->SetDataFormat(&c_dfDIMouse2);
	assert(SUCCEEDED(hr));

	// 排他制御レベルのセット
	hr = devMouse_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	assert(SUCCEEDED(hr));

	// -------------------------------------------------------------------
}


void InputSystem::Update() {
	// keyboard ----------------------------------------------------------

	// 情報の取得開始
	devKeyboard_->Acquire();

	// 入力状態を取得
	keyPre_ = key_;
	devKeyboard_->GetDeviceState(sizeof(key_), key_.data());

	// -------------------------------------------------------------------


	// mouse -------------------------------------------------------------

	// 情報の取得開始
	devMouse_->Acquire();

	// 入力状態を取得
	mousePre_ = mouse_;
	devMouse_->GetDeviceState(sizeof(mouse_), &mouse_);

	POINT point{};
	GetCursorPos(&point);
	ScreenToClient(hwnd_, &point);
	mousePosition_ = {static_cast<float>(point.x), static_cast<float>(point.y)};

	// -------------------------------------------------------------------
}


bool InputSystem::PushKey(BYTE keyNumber) const {
	return key_[keyNumber] != 0;
}


bool InputSystem::TriggerKey(BYTE keyNumber) const {
	return key_[keyNumber] != 0 && keyPre_[keyNumber] == 0;
}

bool InputSystem::PressMouse(MouseButton buttonNumber) const {
	return mouse_.rgbButtons[buttonNumber] != 0;
}


bool InputSystem::TriggerMouse(MouseButton buttonNumber) const {
	return mouse_.rgbButtons[buttonNumber] != 0 && mousePre_.rgbButtons[buttonNumber] == 0;
}


InputSystem::MouseMove InputSystem::GetMouseMove() const {
	return MouseMove{
		mouse_.lX,
		mouse_.lY,
		mouse_.lZ
	};
}


int32_t InputSystem::GetWheel() const {
	return static_cast<int32_t>(mouse_.lZ);
}

// ==================================================================================== public func



// private func ===================================================================================

InputSystem::~InputSystem() {
	if (devKeyboard_) {
		devKeyboard_->Unacquire();
	}

	if (devMouse_) {
		devMouse_->Unacquire();
	}
}

// =================================================================================== private func