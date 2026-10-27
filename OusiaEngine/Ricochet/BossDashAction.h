#pragma once

#include "BossAction.h"
#include "WarningLine.h"

#include <cstdint>


namespace ou {

// 反射突進（第二形態）。予兆線を出してから壁で反射しながら突進し、終わったら長く止まる
class BossDashAction final: public BossAction {
public: // メンバ関数

	void Begin(Boss &boss) override;
	bool Update(Boss &boss, float dt) override;
	void End(Boss &boss) override;


private: // インナークラス

	enum class Step : uint8_t {
		Warning,
		Dashing,
		Recovering,
	};


private: // メンバ変数

	Step step_ = Step::Warning;
	float timer_ = 0.0f;
	int bounceCount_ = 0;
	Vector2 direction_;
	WarningLine line_;
};

} // namespace ou
