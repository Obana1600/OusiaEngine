#pragma once

#include "Math/Vector2.h"
#include "Scene/Component.h"


namespace ou {

// 前方宣言
class ParticleSystem;
class Player;

// プレイヤーに付いて回るパーティクル。
// 溜め中は周りから粒が集まり、中速以上で動いている間はトレイル（光の帯と、こぼれる火の粉）を引く
class PlayerParticles final: public Component {
public: // メンバ関数

	void Start() override;
	void LateUpdate() override;


private: // メンバ変数

	Player *player_ = nullptr;
	ParticleSystem *gather_ = nullptr;
	ParticleSystem *trail_ = nullptr;
	ParticleSystem *embers_ = nullptr;

	Vector2 lastPosition_;
	// 前のフレームから持ち越した、次の粒までの距離
	float trailCarry_ = 0.0f;
	float emberCarry_ = 0.0f;


private: // メンバ関数

	void UpdateGather();
	void UpdateTrail();
};

} // namespace ou
