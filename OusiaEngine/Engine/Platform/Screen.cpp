#include "Screen.h"

using namespace ou;


uint32_t ou::Screen::GetWidth() {
	return sWidth_;
}


uint32_t ou::Screen::GetHeight() {
	return sHeight_;
}


void ou::Screen::SetFullScreenMode([[maybe_unused]] FullScreenMode mode) {
#ifndef WITH_EDITOR
	if (sWindow_) { sWindow_->SetFullScreenMode(mode); }
#endif
}


FullScreenMode ou::Screen::GetFullScreenMode() {
	return sWindow_ ? sWindow_->GetFullScreenMode() : FullScreenMode::Windowed;
}


void ou::Screen::SetFullScreen(bool fullScreen) {
	SetFullScreenMode(fullScreen ? FullScreenMode::FullScreenWindow : FullScreenMode::Windowed);
}


bool ou::Screen::IsFullScreen() {
	return GetFullScreenMode() == FullScreenMode::FullScreenWindow;
}


void ou::Screen::SetResolution([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height, [[maybe_unused]] FullScreenMode mode) {
#ifndef WITH_EDITOR
	if (!sWindow_ || width == 0 || height == 0) { return; }

	sWindow_->SetFullScreenMode(FullScreenMode::Windowed);

	const auto hwnd = sWindow_->Hwnd();
	const auto dpi = GetDpiForWindow(hwnd);
	RECT rect{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
	AdjustWindowRectExForDpi(&rect, static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE)), FALSE, static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE)), dpi);
	SetWindowPos(hwnd, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	sWindow_->SetFullScreenMode(mode);
#endif
}


void ou::Screen::SetSize(uint32_t width, uint32_t height) {
	sWidth_ = width;
	sHeight_ = height;
}
