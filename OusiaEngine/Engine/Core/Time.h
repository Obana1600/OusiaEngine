#pragma once

#include <cstdint>


namespace ou {

// 前方宣言
class Engine;
class EditorApplication;
class SceneManager;

class Time final {
	friend class Engine;
	friend class EditorApplication;
	friend class SceneManager;

public: // 静的メンバ関数

	Time() = delete;

	static float GetDeltaTime();
	static float GetUnscaledDeltaTime();
	static float GetTime();
	static double GetTimeAsDouble();
	static uint64_t GetFrameCount();

	static void SetTimeScale(float timeScale);
	static float GetTimeScale();

	static void SetFixedDeltaTime(float fixedDeltaTime);
	static float GetFixedDeltaTime();
	static float GetFixedTime();
	static double GetFixedTimeAsDouble();
	static bool IsInFixedTimeStep();


private: // 静的メンバ変数

	static inline float sDeltaTime_ = 0.0f;
	static inline float sUnscaledDeltaTime_ = 0.0f;
	static inline double sTime_ = 0.0;
	static inline uint64_t sFrameCount_ = 0;
	static inline float sTimeScale_ = 1.0f;

	static inline float sFixedDeltaTime_ = 0.02f;
	static inline double sFixedTime_ = 0.0;
	static inline bool sIsInFixedTimeStep_ = false;


private: // 静的メンバ関数

	static void Advance(float unscaledDeltaTime);

	static bool AdvanceFixedTime();

	static void ResetForPlayMode();
};

} // namespace ou
