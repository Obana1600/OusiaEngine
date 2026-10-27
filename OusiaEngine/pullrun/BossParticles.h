#pragma once

#include "Scene/Component.h"


namespace ou {

// 前方宣言
class Boss;
class ParticleSystem;

// ボスに付いて回るパーティクル。
// 攻撃の予兆では周りから力が集まり、第二形態では体から赤い火の粉が立ちのぼる
class BossParticles final: public Component {
public: // メンバ関数

	void Start() override;
	void LateUpdate() override;


private: // メンバ変数

	Boss *boss_ = nullptr;
	ParticleSystem *gather_ = nullptr;
	ParticleSystem *aura_ = nullptr;
	int lastPhase_ = 1;
	bool wasDefeated_ = false;
};

} // namespace ou
