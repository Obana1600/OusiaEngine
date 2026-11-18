#pragma once


namespace ou {

// Unity の Project Settings > Time（ProjectSettings/TimeManager.json）
// Play を始めるとき・止めたときに Time へ入れ直すので、スクリプトで Time.timeScale などを変えても、ここの値は変わらない
class TimeManager final {
	friend class Engine;

public: // 静的メンバ関数

	TimeManager() = delete;

	// 変えるとすぐ Time にも入れる（ファイルに書くのは Save）
	static void SetFixedTimestep(float fixedTimestep);
	static float GetFixedTimestep();

	static void SetMaximumAllowedTimestep(float maximumAllowedTimestep);
	static float GetMaximumAllowedTimestep();

	static void SetTimeScale(float timeScale);
	static float GetTimeScale();

	static void Save();

	// 設定の値を Time に入れる（起動したとき・Play を始めるとき・止めたとき）
	static void Apply();


private: // 静的メンバ変数

	static inline float sFixedTimestep_ = 0.02f;
	static inline float sMaximumAllowedTimestep_ = 1.0f / 3.0f;
	static inline float sTimeScale_ = 1.0f;


private: // 静的メンバ関数

	static void Load();
};

} // namespace ou
