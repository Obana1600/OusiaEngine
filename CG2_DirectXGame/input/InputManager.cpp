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
	m_hwnd = WinApp::GetInstance()->GetHwnd();

	// DirectInputの初期化
	HRESULT hr = DirectInput8Create(
		WinApp::GetInstance()->GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8,
		reinterpret_cast<void **>(m_directInput.ReleaseAndGetAddressOf()), nullptr);
	assert(SUCCEEDED(hr));


	// keuboard ----------------------------------------------------------

	// デバイスの生成
	hr = m_directInput->CreateDevice(GUID_SysKeyboard, m_devKeyboard.GetAddressOf(), NULL);
	assert(SUCCEEDED(hr));

	// 入力データ形式のセット
	hr = m_devKeyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(hr));

	// 排他制御レベルのセット
	hr = m_devKeyboard->SetCooperativeLevel(m_hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(hr));

	// -------------------------------------------------------------------


	// mouse -------------------------------------------------------------

	// マウスの生成
	hr = m_directInput->CreateDevice(GUID_SysMouse, m_devMouse.GetAddressOf(), NULL);
	assert(SUCCEEDED(hr));

	// 入力データ形式のセット
	hr = m_devMouse->SetDataFormat(&c_dfDIMouse2);
	assert(SUCCEEDED(hr));

	// 排他制御レベルのセット
	hr = m_devMouse->SetCooperativeLevel(m_hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	assert(SUCCEEDED(hr));

	// -------------------------------------------------------------------
}


void InputManager::Update() {
	// keyboard ----------------------------------------------------------

	// 情報の取得開始
	m_devKeyboard->Acquire();

	// 入力状態を取得
	m_keyPre = m_key;
	m_devKeyboard->GetDeviceState(sizeof(m_key), m_key.data());

	// -------------------------------------------------------------------


	// mouse -------------------------------------------------------------

	// 情報の取得開始
	m_devMouse->Acquire();

	// 入力状態を取得
	m_mousePre = m_mouse;
	m_devMouse->GetDeviceState(sizeof(m_mouse), &m_mouse);

	POINT point{};
	GetCursorPos(&point);
	ScreenToClient(m_hwnd, &point);
	m_mousePosition = { static_cast<float>(point.x), static_cast<float>(point.y) };

	// -------------------------------------------------------------------
}


bool InputManager::PushKey(BYTE keyNumber) const {
	return m_key[keyNumber] != 0;
}


bool InputManager::TriggerKey(BYTE keyNumber) const {
	return m_key[keyNumber] != 0
		&& m_keyPre[keyNumber] == 0;
}

bool InputManager::PressMouse(MouseButton buttonNumber) const {
	return m_mouse.rgbButtons[buttonNumber] != 0;
}


bool InputManager::TriggerMouse(MouseButton buttonNumber) const {
	return m_mouse.rgbButtons[buttonNumber] != 0
		&& m_mousePre.rgbButtons[buttonNumber] == 0;
}


InputManager::MouseMove InputManager::GetMouseMove() const {
	return MouseMove{
		m_mouse.lX,
		m_mouse.lY,
		m_mouse.lZ
	};
}


int32_t InputManager::GetWheel() const {
	return static_cast<int32_t>(m_mouse.lZ);
}

// ==================================================================================== public func



// private func ===================================================================================

InputManager::~InputManager() {
	if (m_devKeyboard) {
		m_devKeyboard->Unacquire();
	}

	if (m_devMouse) {
		m_devMouse->Unacquire();
	}
}

// =================================================================================== private func