#pragma once

#include "Math/Vector2.h"
#include "Platform/Window.h"


namespace ou {

// 前方宣言
class Texture;

enum class CursorLockMode {
	None,	  // 自由に動かせる
	Locked,	  // ゲーム画面の中心に固定して隠す
	Confined, // ゲーム画面の中に閉じ込める
};

enum class CursorMode {
	Auto,
	ForceSoftware,
};


class Cursor final {
	friend class Engine;
	friend class EditorApplication;

public: // 静的メンバ関数

	Cursor() = delete;

	static bool IsVisible();
	static void SetVisible(bool visible);

	static CursorLockMode GetLockState();
	static void SetLockState(CursorLockMode mode);

	static void SetCursor(const Texture *texture, const Vector2 &hotspot, CursorMode cursorMode);


private: // 静的メンバ変数

	static inline Window *sWindow_ = nullptr;
	static inline Window::MessageHandlerId sMessageHandlerId_ = 0;

	static inline bool sVisible_ = true;
	static inline CursorLockMode sLockState_ = CursorLockMode::None;
	static inline bool sIsClipping_ = false;

	static inline HCURSOR sCustomCursor_ = nullptr;
	static inline bool sIsControlling_ = false;
	static inline HCURSOR sShape_ = nullptr;

#ifdef WITH_EDITOR
	static inline Vector2 sGameViewOrigin_{};
	static inline bool sGameViewHasFocus_ = false;
	static inline bool sGameViewIsHovered_ = false;
	static inline bool sIsSuspended_ = false;
#endif


private: // 静的メンバ関数

	static void Initialize(Window *window);
	static void Finalize();
	static void Update();

	static void ResetForPlayMode();

#ifdef WITH_EDITOR
	static void SetGameView(const Vector2 &origin, bool hasFocus, bool isHovered);
#endif
};

} // namespace ou
