#pragma once

#include "BossAction.h"
#include "WarningLine.h"


namespace ou {

// ランダムな位置へ移動して静止する。移動前に移動先への予兆線を出す
class BossMoveAction final: public BossAction {
public: // メンバ関数

	void Begin(Boss &boss) override;
	bool Update(Boss &boss, float dt) override;
	void End(Boss &boss) override;


private: // メンバ変数

	Vector2 from_;
	Vector2 to_;
	float timer_ = 0.0f;
	WarningLine line_;


private: // メンバ関数

	static Vector2 ChooseDestination(const Boss &boss);
};

} // namespace ou
