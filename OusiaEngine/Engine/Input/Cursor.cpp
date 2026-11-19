#include "Cursor.h"
#include "Diagnostics/Log.h"
#include "Input/Input.h"
#include "Platform/Screen.h"
#include "Render/Asset/Texture.h"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#ifdef USE_IMGUI
#include <imgui/imgui.h>
#endif

using namespace ou;


namespace {

RECT CalcGameRect(HWND hwnd, const Vector2 &origin) {
	POINT topLeft{static_cast<LONG>(origin.x), static_cast<LONG>(origin.y)};
	ClientToScreen(hwnd, &topLeft);
	return {
		topLeft.x,
		topLeft.y,
		topLeft.x + static_cast<LONG>(Screen::GetWidth()),
		topLeft.y + static_cast<LONG>(Screen::GetHeight())};
}

RECT CalcCenterRect(const RECT &rect) {
	const LONG x = (rect.left + rect.right) / 2;
	const LONG y = (rect.top + rect.bottom) / 2;
	return {x, y, x + 1, y + 1};
}

bool IsCursorInClient(HWND hwnd) {
	POINT point{};
	GetCursorPos(&point);
	ScreenToClient(hwnd, &point);
	RECT client{};
	GetClientRect(hwnd, &client);
	return PtInRect(&client, point) != FALSE;
}

HCURSOR LoadArrowCursor() {
	return LoadCursorW(nullptr, IDC_ARROW);
}

HCURSOR CreateCursorFromPixels(const std::vector<uint8_t> &rgba, uint32_t width, uint32_t height, const Vector2 &hotspot) {
	BITMAPV5HEADER header{};
	header.bV5Size = sizeof(header);
	header.bV5Width = static_cast<LONG>(width);
	header.bV5Height = -static_cast<LONG>(height);
	header.bV5Planes = 1;
	header.bV5BitCount = 32;
	header.bV5Compression = BI_BITFIELDS;
	header.bV5RedMask = 0x00FF0000;
	header.bV5GreenMask = 0x0000FF00;
	header.bV5BlueMask = 0x000000FF;
	header.bV5AlphaMask = 0xFF000000;

	void *bits = nullptr;
	const auto dc = GetDC(nullptr);
	const auto color = CreateDIBSection(dc, reinterpret_cast<const BITMAPINFO *>(&header), DIB_RGB_COLORS, &bits, nullptr, 0);
	ReleaseDC(nullptr, dc);
	if (!color) { return nullptr; }

	auto *dst = static_cast<uint8_t *>(bits);
	const auto pixelCount = static_cast<size_t>(width) * height;
	for (size_t i = 0; i < pixelCount; i++) {
		dst[i * 4 + 0] = rgba[i * 4 + 2];
		dst[i * 4 + 1] = rgba[i * 4 + 1];
		dst[i * 4 + 2] = rgba[i * 4 + 0];
		dst[i * 4 + 3] = rgba[i * 4 + 3];
	}

	const std::vector<uint8_t> maskBits(((width + 15) / 16) * 2 * height, 0);
	const auto mask = CreateBitmap(static_cast<int>(width), static_cast<int>(height), 1, 1, maskBits.data());

	ICONINFO info{};
	info.fIcon = FALSE;
	info.xHotspot = static_cast<DWORD>(std::clamp(hotspot.x, 0.0f, static_cast<float>(width - 1)));
	info.yHotspot = static_cast<DWORD>(std::clamp(hotspot.y, 0.0f, static_cast<float>(height - 1)));
	info.hbmMask = mask;
	info.hbmColor = color;
	const auto cursor = CreateIconIndirect(&info);

	DeleteObject(mask);
	DeleteObject(color);
	return cursor;
}

} // namespace



bool ou::Cursor::IsVisible() {
	return sVisible_;
}


void ou::Cursor::SetVisible(bool visible) {
	sVisible_ = visible;
}


CursorLockMode ou::Cursor::GetLockState() {
	return sLockState_;
}


void ou::Cursor::SetLockState(CursorLockMode mode) {
	sLockState_ = mode;
}


void ou::Cursor::SetCursor(const Texture *texture, const Vector2 &hotspot, CursorMode) {
	HCURSOR next = nullptr;
	if (texture) {
		if (!texture->IsReadable()) {
			Log::Error("カーソルにできるのは、Texture Type が Cursor のテクスチャだけです: {}", texture->GetName());
			return;
		}
		next = CreateCursorFromPixels(texture->pixels_, texture->GetWidth(), texture->GetHeight(), hotspot);
		if (!next) {
			Log::Error("カーソルを作れませんでした: {}", texture->GetName());
			return;
		}
	}

	const auto previous = std::exchange(sCustomCursor_, next);
	if (!previous) { return; }

	if (GetCursor() == previous) { ::SetCursor(LoadArrowCursor()); }
	DestroyCursor(previous);
}


void ou::Cursor::Initialize(Window *window) {
	sWindow_ = window;

	sMessageHandlerId_ = window->AddMessageHandler(
		[](HWND, UINT msg, WPARAM, LPARAM lparam) -> std::optional<LRESULT> {
			if (msg == WM_SETCURSOR && LOWORD(lparam) == HTCLIENT && sIsControlling_) {
				::SetCursor(sShape_);
				return TRUE;
			}
			return std::nullopt;
		});
}


void ou::Cursor::Finalize() {
	if (sIsClipping_) {
		ClipCursor(nullptr);
		sIsClipping_ = false;
	}

	if (sCustomCursor_) {
		::SetCursor(LoadArrowCursor());
		DestroyCursor(sCustomCursor_);
		sCustomCursor_ = nullptr;
	}

	if (sWindow_) { sWindow_->RemoveMessageHandler(sMessageHandlerId_); }
	sWindow_ = nullptr;
}


void ou::Cursor::Update() {
	if (!sWindow_) { return; }
	const auto hwnd = sWindow_->Hwnd();

	auto isFocused = (GetForegroundWindow() == hwnd) && !sWindow_->IsMinimized();
	auto isSuspended = false;
	auto origin = Vector2::Zero;
	auto isHovered = IsCursorInClient(hwnd);
#ifdef WITH_EDITOR
	if (Input::GetKeyDown(KeyCode::Escape)) { sIsSuspended_ = true; }
	if (sGameViewIsHovered_ && Input::GetMouseButtonDown(0)) { sIsSuspended_ = false; }

	isFocused = isFocused && sGameViewHasFocus_;
	isSuspended = sIsSuspended_;
	origin = sGameViewOrigin_;
	isHovered = sGameViewIsHovered_;
#endif

	const auto isLockActive = isFocused && !isSuspended;

	const auto wantsClip = isLockActive && (sLockState_ != CursorLockMode::None);
	if (wantsClip) {
		const auto gameRect = CalcGameRect(hwnd, origin);
		const auto clipRect = (sLockState_ == CursorLockMode::Locked) ? CalcCenterRect(gameRect) : gameRect;
		ClipCursor(&clipRect);
	} else if (sIsClipping_) {
		ClipCursor(nullptr);
	}
	sIsClipping_ = wantsClip;

	const auto isControlling = isFocused && isHovered;
	const auto wantsHide = isLockActive && (!sVisible_ || sLockState_ == CursorLockMode::Locked);
	const auto shape = wantsHide ? nullptr : (sCustomCursor_ ? sCustomCursor_ : LoadArrowCursor());

#ifdef USE_IMGUI
	auto &io = ImGui::GetIO();
	if (isControlling) {
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	} else {
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
	}
#endif

	if (isControlling && (!sIsControlling_ || shape != sShape_)) {
		::SetCursor(shape);
	}
	sIsControlling_ = isControlling;
	sShape_ = shape;
}


void ou::Cursor::ResetForPlayMode() {
	sVisible_ = true;
	sLockState_ = CursorLockMode::None;
	SetCursor(nullptr, Vector2::Zero, CursorMode::Auto);
#ifdef WITH_EDITOR
	sIsSuspended_ = false;
#endif
}


#ifdef WITH_EDITOR
void ou::Cursor::SetGameView(const Vector2 &origin, bool hasFocus, bool isHovered) {
	sGameViewOrigin_ = origin;
	sGameViewHasFocus_ = hasFocus;
	sGameViewIsHovered_ = isHovered;
}
#endif
