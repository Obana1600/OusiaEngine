#pragma once

#include "Scene/Component.h"


namespace ou {

// 溜め中のスローと、ヒットストップを Time のスケールにまとめて反映する
class TimeControl final: public Component {
public: // 静的メンバ関数

	static void RequestHitStop(float seconds);
	static void SetSlowMotion(bool isSlow);


public: // メンバ関数

	void Awake() override;
	void Update() override;
	void OnDestroy() override;


private: // 静的メンバ変数

	static inline float sHitStopRemaining_ = 0.0f;
	static inline bool sIsSlow_ = false;
};

} // namespace ou
