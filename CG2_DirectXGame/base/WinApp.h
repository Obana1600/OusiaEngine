#pragma once

#include <Windows.h>
#include <string>


class WinApp {
public: // 静的メンバ変数

	static const int kWindowWidth = 1280;
	static const int kWindowHeight = 720;


public: // 静的メンバ関数

	static WinApp *GetInstance();

	static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


public: // メンバ関数

	void CreateGameWindow(
		const std::wstring &title = L"DirectXGame",
		UINT windowStyle = WS_OVERLAPPEDWINDOW,
		int32_t clientWidth = kWindowWidth,
		int32_t clientHeight = kWindowHeight);

	void TerminateGameWindow();


	HINSTANCE GetHInstance() const {
		return m_wndClass.hInstance;
	}

	HWND GetHwnd() const {
		return m_hwnd;
	}


private: // メンバ変数

	WNDCLASS m_wndClass{};
	HWND m_hwnd{};


private: // メンバ関数

	WinApp() = default;
	~WinApp() = default;
	WinApp(const WinApp &) = delete;
	const WinApp &operator=(const WinApp &) = delete;

}; // WinApp