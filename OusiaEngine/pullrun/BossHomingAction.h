#pragma once

#include "BossAction.h"


namespace ou {

// 追尾弾を数発、扇状に撃つ。第二形態は発射数と追尾速度が上がる
class BossHomingAction final: public BossAction {
public: // メンバ関数

	bool Update(Boss &boss, float dt) override;
	void End(Boss &boss) override;


private: // メンバ変数

	float timer_ = 0.0f;
	int firedCount_ = 0;
};

} // namespace ou
