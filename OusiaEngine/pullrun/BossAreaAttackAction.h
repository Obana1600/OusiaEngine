#pragma once

#include "BossAction.h"


namespace ou {

// 前方宣言
class FilledRing;
class GameObject;

// 近接エリア攻撃（第二形態）。ボスの周りの範囲表示が徐々に満ち、満ちた瞬間に範囲内のプレイヤーにダメージを与える
class BossAreaAttackAction final: public BossAction {
public: // メンバ関数

	void Begin(Boss &boss) override;
	bool Update(Boss &boss, float dt) override;
	void End(Boss &boss) override;


private: // メンバ変数

	float timer_ = 0.0f;
	bool hasExploded_ = false;
	GameObject *root_ = nullptr;
	FilledRing *outline_ = nullptr;
	FilledRing *fill_ = nullptr;
};

} // namespace ou
