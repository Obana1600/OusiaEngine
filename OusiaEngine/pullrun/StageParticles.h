#pragma once

#include "Math/Color.h"
#include "Math/Vector2.h"
#include "Scene/Component.h"


namespace ou {

// 前方宣言
class ParticleSystem;

// ステージの奥で、中央へ常に集まっていく集中線。第二形態では速く、多く、赤くなる
class StageParticles final: public Component {
public: // メンバ関数

	void Start() override;
	void Update() override;


private: // メンバ変数

	ParticleSystem *lines_ = nullptr;
	// 線を出す数の端数の持ち越し
	float spawnCarry_ = 0.0f;
	int lastPhase_ = 1;


private: // メンバ関数

	// ステージ中央を囲む輪から、中央へ向かう線を 1 本出す
	void SpawnLine(float speed, float lifetime, const Color &color);
};

} // namespace ou
