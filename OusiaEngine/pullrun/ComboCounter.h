#pragma once

#include "Scene/Component.h"


namespace ou {

// コンボ。ボスへのヒットと巨大弾の破壊で加算し、一定時間ヒットがないとリセットする（被弾や停止ではリセットしない）
class ComboCounter final: public Component {
public: // 静的メンバ関数

	static ComboCounter *GetInstance() { return sInstance_; }


public: // メンバ関数

	void Awake() override;
	void Update() override;
	void OnDestroy() override;

	void Add();

	int GetCombo() const { return combo_; }
	int GetMaxCombo() const { return maxCombo_; }
	// リセットまでの残り時間の割合（1 → 0）
	float GetRemainingRatio() const;


private: // 静的メンバ変数

	static inline ComboCounter *sInstance_ = nullptr;


private: // メンバ変数

	int combo_ = 0;
	int maxCombo_ = 0;
	float timer_ = 0.0f;
};

} // namespace ou
