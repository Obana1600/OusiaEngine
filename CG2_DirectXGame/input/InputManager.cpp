#include "InputManager.h"
#include "base/WinApp.h"
#include <cassert>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")


// public func ====================================================================================

InputManager *InputManager::GetInstance() {
	static InputManager instance;
	return &instance;
}


void InputManager::Initialize() {
	// hwndの受け取り
	hwnd_ = WinApp::GetInstance()->GetHwnd();

	// DirectInputの初期化
	HRESULT hr = DirectInput8Create(
		WinApp::GetInstance()->GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8,
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
	hr = devKeyboard_->SetCooperativeLevel(hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
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
	hr = devMouse_->SetCooperativeLevel(hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	assert(SUCCEEDED(hr));

	// -------------------------------------------------------------------
}


void InputManager::Update() {
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


bool InputManager::PushKey(BYTE keyNumber) const {
	return key_[keyNumber] != 0;
}


bool InputManager::TriggerKey(BYTE keyNumber) const {
	return key_[keyNumber] != 0 && keyPre_[keyNumber] == 0;
}

bool InputManager::PressMouse(MouseButton buttonNumber) const {
	return mouse_.rgbButtons[buttonNumber] != 0;
}


bool InputManager::TriggerMouse(MouseButton buttonNumber) const {
	return mouse_.rgbButtons[buttonNumber] != 0 && mousePre_.rgbButtons[buttonNumber] == 0;
}


InputManager::MouseMove InputManager::GetMouseMove() const {
	return MouseMove{
		mouse_.lX,
		mouse_.lY,
		mouse_.lZ
	};
}


int32_t InputManager::GetWheel() const {
	return static_cast<int32_t>(mouse_.lZ);
}

// ==================================================================================== public func



// private func ===================================================================================

InputManager::~InputManager() {
	if (devKeyboard_) {
		devKeyboard_->Unacquire();
	}

	if (devMouse_) {
		devMouse_->Unacquire();
	}
}

// =================================================================================== private func