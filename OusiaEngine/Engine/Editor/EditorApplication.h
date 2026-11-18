#pragma once

#include <cstdint>


namespace ou {

class EditorApplication final {
	friend class Engine;

public: // メンバ関数

	static bool IsPlaying();

	static void EnterPlayMode();

	static void ExitPlayMode();

	static void ApplyPendingPlayModeChange();

	// Unity の EditorApplication.isPaused。Play 中でなくても切り替えられ、Play する前に入れておくと、最初の 1 フレームだけ動かして止まる
	// Stop しても戻さない（Unity と同じく、次の Play も止まった状態で始まる）
	static void SetPaused(bool isPaused);
	static bool IsPaused();

	// Unity の EditorApplication.Step。Play 中だけ。止めて、1 フレームだけ進める
	static void Step();

	// このフレームは、Pause でゲームの更新を止めているか（Step のフレームは止めない）
	static bool IsGameFrameSkipped();


private: // インナークラス

	enum class Request : uint8_t {
		None,
		Enter,
		Exit,
	};


private: // 静的メンバ変数

	static inline bool sIsPlaying_ = false;
	static inline Request sRequest_ = Request::None;

	static inline bool sIsPaused_ = false;
	static inline bool sIsStepRequested_ = false;
	// このフレームの分（BeginGameFrame で決める）
	static inline bool sSkipsGameFrame_ = false;
	static inline bool sIsStepFrame_ = false;
	// 止めている間は、音も止める
	static inline bool sIsAudioPaused_ = false;


private: // 静的メンバ関数

	static void ReloadScriptsIfChanged();

	// Engine::Update から、Time を進める前に呼ぶ。このフレームにゲームを動かすかを決め、時間を進める
	static void BeginGameFrame(float unscaledDeltaTime);
};

} // namespace ou
