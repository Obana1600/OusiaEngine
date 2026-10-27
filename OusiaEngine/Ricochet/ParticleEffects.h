#pragma once

#include "Math/Color.h"
#include "Math/Vector2.h"
#include "Scene/Component.h"


namespace ou {

// 前方宣言
class ParticleSystem;

// 一度だけはじけるパーティクル（衝突・爆発・輪）。使い回すエミッタを持ち、出す場所へ動かしてから Emit する
class ParticleEffects final: public Component {
public: // 静的メンバ関数

	// 体当たりなどの強い衝突。direction へ火花が飛び、閃光とひし形の破片が散る。strength は 0〜1
	static void PlayHit(const Vector2 &position, const Vector2 &direction, const Color &color, float strength);
	// 全方向に散る爆発。strength は 0〜1（1 を超えると大きくなる）
	static void PlayExplosion(const Vector2 &position, const Color &color, float strength);
	// 壁に当たったときなどの小さな火花
	static void PlaySparks(const Vector2 &position, const Vector2 &direction, const Color &color, int count);
	// xy 平面に広がる輪。radius まで広がって消える
	static void PlayRing(const Vector2 &position, const Color &color, float radius);


public: // メンバ関数

	void Awake() override;
	void OnDestroy() override;


private: // 静的メンバ変数

	static inline ParticleEffects *sInstance_ = nullptr;


private: // メンバ変数

	ParticleSystem *sparks_ = nullptr;
	ParticleSystem *flash_ = nullptr;
	ParticleSystem *shards_ = nullptr;
	ParticleSystem *ring_ = nullptr;


private: // メンバ関数

	// coneAngle が 180 以上なら全方向
	void EmitSparks(const Vector2 &position, const Vector2 &direction, const Color &color, int count, float speed, float coneAngle);
	void EmitFlash(const Vector2 &position, const Color &color, float size);
	void EmitShards(const Vector2 &position, const Color &color, int count, float speed);
};

} // namespace ou
