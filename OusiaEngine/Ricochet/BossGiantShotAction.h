#pragma once

#include "BossAction.h"


namespace ou {

// 壁で反射する巨大弾を撃つ。第二形態は発射数とサイズが上がる
class BossGiantShotAction final: public BossAction {
public: // メンバ関数

	bool Update(Boss &boss, float dt) override;
	void End(Boss &boss) override;


private: // メンバ変数

	float timer_ = 0.0f;
	bool hasFired_ = false;
};

} // namespace ou
