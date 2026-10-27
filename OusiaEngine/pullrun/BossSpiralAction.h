#pragma once

#include "BossAction.h"


namespace ou {

// 渦巻き弾（第二形態）。回転しながら小弾を数方向に撃ち続ける
class BossSpiralAction final: public BossAction {
public: // メンバ関数

	void Begin(Boss &boss) override;
	bool Update(Boss &boss, float dt) override;
	void End(Boss &boss) override;


private: // メンバ変数

	float timer_ = 0.0f;
	float fireTimer_ = 0.0f;
	float angle_ = 0.0f;
	float spinSign_ = 1.0f;
};

} // namespace ou
