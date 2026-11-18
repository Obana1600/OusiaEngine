#include "Window.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"

using namespace ou;


namespace {

// ウィンドウクラスの名前
constexpr const wchar_t *kClassName = L"Engine Window";

// ウィンドウの最小サイズ
constexpr int kMinClientWidth = 640;
constexpr int kMinClientHeight = 360;

// ウィンドウスタイル
constexpr DWORD kStyle = WS_OVERLAPPEDWINDOW;
constexpr DWORD kExStyle = 0;

// 基準 DPI
constexpr int kBaseDpi = 96;

} // namespace



std::unique_ptr<Window> Window::Create(const WindowDesc &desc) {
	std::unique_ptr<Window> w(new Window());

	// 可変 DPI に対応させる
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	const auto instance = GetModuleHandleW(nullptr);

	// ウィンドウクラスの登録
	WNDCLASSEXW wndClass{};
	wndClass.cbSize = sizeof(wndClass);
	wndClass.lpfnWndProc = StaticWindowProc;
	wndClass.hInstance = instance;
	wndClass.hCursor = LoadCursorW(nullptr, IDC_ARROW); // TODO: カーソルの変更機能つけたい
	wndClass.lpszClassName = kClassName;
	RegisterClassExW(&wndClass);

	// DPI に合わせてクライアント領域を拡大する
	const auto dpi = GetDpiForSystem();
	RECT rect{
		0, 0,
		MulDiv(static_cast<int>(desc.width), static_cast<int>(dpi), kBaseDpi),
		MulDiv(static_cast<int>(desc.height), static_cast<int>(dpi), kBaseDpi)};

	// ウィンドウ全体の大きさにする
	AdjustWindowRectExForDpi(&rect, kStyle, FALSE, 0, dpi);
	const auto wndWidth = rect.right - rect.left;
	const auto wndHeight = rect.bottom - rect.top;

	// ウィンドウの作成
	w->hwnd_ = CreateWindowExW(
		kExStyle,
		kClassName,
		Utf8ToWide(desc.title).c_str(),
		kStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, // ウィンドウ位置は OS に任せる
		wndWidth, wndHeight,
		nullptr, // 親ウィンドウなし
		nullptr, // メニューなし
		instance,
		w.get());

	if (!w->hwnd_) {
		Log::Error("HWND の作成に失敗しました");
		return nullptr;
	}

	if (desc.windowedRect) {
		WINDOWPLACEMENT placement{.length = sizeof(WINDOWPLACEMENT)};
		GetWindowPlacement(w->hwnd_, &placement);
		placement.showCmd = SW_HIDE;
		placement.rcNormalPosition = *desc.windowedRect;

		// モニタを外したなどで、どの画面にも入らないときは OS に任せる
		if (MonitorFromRect(&*desc.windowedRect, MONITOR_DEFAULTTONULL)) {
			SetWindowPlacement(w->hwnd_, &placement);
		}
	}

	// ウィンドウの表示
	w->SetFullScreenMode(desc.fullScreenMode);

	return w;
}


Window::~Window() {
	if (hwnd_) {
		// StaticWindowProc での設定を削除
		SetWindowLongPtrW(hwnd_, GWLP_USERDATA, 0);
		// ウィンドウを破棄
		DestroyWindow(hwnd_);
	}

	// ウィンドウクラスの登録を解除
	UnregisterClassW(kClassName, GetModuleHandleW(nullptr));
}


void Window::PumpMessages() {
	MSG msg{};

	while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}


Window::MessageHandlerId Window::AddMessageHandler(MessageHandler handler) {
	const auto id = nextHandlerId_++;
	handlers_.push_back({std::move(handler), id});
	return id;
}


void Window::RemoveMessageHandler(MessageHandlerId id) {
	std::erase_if(handlers_, [id](const HandlerEntry &entry) {
		return entry.id == id;
	});
}


void ou::Window::SetFullScreenMode(FullScreenMode mode) {
	// TODO: 排他的フルスクリーンの対応
	if (mode == FullScreenMode::ExclusiveFullScreen) {
		mode = FullScreenMode::FullScreenWindow;
	}

	if (mode == FullScreenMode::FullScreenWindow) {
		if (isFullScreen_) { return; }

		// 戻すときのために、今の位置と状態を覚える
		GetWindowPlacement(hwnd_, &windowedPlacement_);

		// 枠をなくして、ウィンドウがあるモニタ全体に広げる
		MONITORINFO monitor{.cbSize = sizeof(MONITORINFO)};
		GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &monitor);
		const auto &rect = monitor.rcMonitor;

		SetWindowLongPtrW(hwnd_, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(
			hwnd_, HWND_TOP,
			rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
			SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

		isFullScreen_ = true;
		return;
	}

	// 全画面から戻すときは、枠と前の位置を戻す
	if (isFullScreen_) {
		SetWindowLongPtrW(hwnd_, GWL_STYLE, kStyle);
		windowedPlacement_.showCmd = SW_HIDE;
		SetWindowPlacement(hwnd_, &windowedPlacement_);
		SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
		isFullScreen_ = false;
	}

	ShowWindow(hwnd_, mode == FullScreenMode::MaximizedWindow ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
}


FullScreenMode ou::Window::GetFullScreenMode() const {
	if (isFullScreen_) { return FullScreenMode::FullScreenWindow; }
	return IsZoomed(hwnd_) ? FullScreenMode::MaximizedWindow : FullScreenMode::Windowed;
}


void Window::CancelClose() {
	closeRequested_ = false;
}


HWND Window::Hwnd() const {
	return hwnd_;
}


uint32_t Window::ClientWidth() const {
	return clientWidth_;
}


uint32_t Window::ClientHeight() const {
	return clientHeight_;
}


RECT ou::Window::GetWindowedRect() const {
	if (isFullScreen_) { return windowedPlacement_.rcNormalPosition; }

	WINDOWPLACEMENT placement{.length = sizeof(WINDOWPLACEMENT)};
	GetWindowPlacement(hwnd_, &placement);
	return placement.rcNormalPosition;
}


bool Window::IsCloseRequested() const {
	return closeRequested_;
}


bool Window::IsMinimized() const {
	return minimized_;
}


LRESULT Window::StaticWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	Window *self = nullptr;

	// 初めてのウィンドウ作成時に Window* と HWND を紐づける
	if (msg == WM_NCCREATE) {
		const auto *create = reinterpret_cast<CREATESTRUCTW *>(lparam);
		self = static_cast<Window *>(create->lpCreateParams);
		self->hwnd_ = hwnd;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));

	} else {
		// 保存してある Window* を取り出す
		self = reinterpret_cast<Window *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
	}

	// Window が取得できなかったら標準のプロシージャに任せる
	if (!self) {
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	return self->WindowProc(hwnd, msg, wparam, lparam);
}


LRESULT Window::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	// 先に登録された処理を実行
	for (auto &entry : handlers_) {
		if (auto result = entry.handler(hwnd, msg, wparam, lparam)) {
			return *result;
		}
	}

	// メッセージの処理
	switch (msg) {
		case WM_CLOSE: // ウィンドウの終了時
			closeRequested_ = true;
			return 0;

		case WM_SIZE: // ウィンドウサイズの変更時
			clientWidth_ = LOWORD(lparam);
			clientHeight_ = HIWORD(lparam);
			minimized_ = (wparam == SIZE_MINIMIZED);
			resized_ = true;

			return 0;

		case WM_GETMINMAXINFO: // ウィンドウの最小サイズの設定
		{
			const auto dpi = static_cast<int>(GetDpiForWindow(hwnd));
			RECT rect{
				0, 0,
				MulDiv(kMinClientWidth, dpi, kBaseDpi),
				MulDiv(kMinClientHeight, dpi, kBaseDpi)};
			AdjustWindowRectExForDpi(&rect, kStyle, FALSE, 0, static_cast<UINT>(dpi));

			auto *info = reinterpret_cast<MINMAXINFO *>(lparam);
			info->ptMinTrackSize.x = rect.right - rect.left;
			info->ptMinTrackSize.y = rect.bottom - rect.top;
			return 0;
		}

		case WM_DPICHANGED: // ウィンドウの DPI 変更時
		{
			const auto *suggested = reinterpret_cast<RECT *>(lparam);
			SetWindowPos(
				hwnd, nullptr,
				suggested->left, suggested->top,
				suggested->right - suggested->left, suggested->bottom - suggested->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
			return 0;
		}

		case WM_ENTERSIZEMOVE: // ウィンドウのサイズ変更/移動を開始時
			inSizeMove_ = true;
			return 0;

		case WM_EXITSIZEMOVE: // ウィンドウのサイズ変更/移動を終了時
			inSizeMove_ = false;
			return 0;

		case WM_SYSCOMMAND: // システムコマンドの実行時
			if ((wparam & 0xFFF0) == SC_KEYMENU) {
				return 0;
			}
			break;
	}

	// 残りのメッセージは標準のプロシージャに任せる
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}
