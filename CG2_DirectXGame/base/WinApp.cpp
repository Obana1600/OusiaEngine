#include "WinApp.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif


WinApp *WinApp::GetInstance() {
	static WinApp instance{};
	return &instance;
}


LRESULT WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
#ifdef USE_IMGUI
	// imguiでマウス操作をできるようにする
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
#endif

	// メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		case WM_DESTROY: // ウィンドウが破棄された
			PostQuitMessage(0); // OSに対して、アプリの終了を伝える
			return 0;
	}

	// 標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}


void WinApp::CreateGameWindow(const std::wstring &title, UINT windowStyle, int32_t clientWidth, int32_t clientHeight) {
	// ウィンドウプロシージャ
	m_wndClass.lpfnWndProc = WindowProc;
	// ウィンドウクラス名
	m_wndClass.lpszClassName = L"MyEngine";
	// インスタンスハンドル
	m_wndClass.hInstance = GetModuleHandle(nullptr);
	// カーソル
	m_wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	// ウィンドウクラスを登録する
	RegisterClass(&m_wndClass);

	// ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0, 0, clientWidth, clientHeight };
	// クライアント領域をもとに実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウの生成
	m_hwnd = CreateWindow(
		m_wndClass.lpszClassName,		// 利用するクラス名
		title.c_str(),						// タイトルバーの文字
		windowStyle,		// よく見るウィンドウスタイル
		CW_USEDEFAULT,				// 表示X座標（Windowsに任せる）
		CW_USEDEFAULT,				// 表示Y座標（WindowsOSに任せる）
		wrc.right - wrc.left,		// ウィンドウ横幅
		wrc.bottom - wrc.top,		// ウィンドウ縦幅
		nullptr,					// 親ウィンドウハンドル
		nullptr,					// メニューハンドル
		m_wndClass.hInstance,		// インスタンスハンドル
		nullptr						// オプション
	);

	// ウィンドウを表示する
	ShowWindow(m_hwnd, SW_SHOW);
}


void WinApp::TerminateGameWindow() {
	CloseWindow(m_hwnd);
}