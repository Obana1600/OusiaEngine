#pragma once

#include "BossAction.h"


namespace ou {

// 小弾の乱射。プレイヤーの方向へばらつかせて撃つ
class BossBarrageAction final: public BossAction {
public: // メンバ関数

	void Begin(Boss &boss) override;
	bool Update(Boss &boss, float dt) override;
	void End(Boss &boss) override;


private: // メンバ変数

	float timer_ = 0.0f;
	float fireTimer_ = 0.0f;
};

} // namespace ou
