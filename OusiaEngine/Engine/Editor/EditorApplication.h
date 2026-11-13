#pragma once

#include <cstdint>


namespace ou {

class EditorApplication final {
public: // メンバ関数

	static bool IsPlaying();

	static void EnterPlayMode();

	static void ExitPlayMode();

	static void ApplyPendingPlayModeChange();


private: // インナークラス

	enum class Request : uint8_t {
		None,
		Enter,
		Exit,
	};


private: // 静的メンバ変数

	static inline bool sIsPlaying_ = false;
	static inline Request sRequest_ = Request::None;
};

} // namespace ou
