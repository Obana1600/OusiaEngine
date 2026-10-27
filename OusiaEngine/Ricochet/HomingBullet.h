#pragma once

#include "Projectile.h"


namespace ou {

// 前方宣言
class Scene;

// 一定時間プレイヤーを追いかけ、その後はまっすぐ飛ぶ弾。旋回の速さには上限がある
class HomingBullet final: public Projectile {
public: // 静的メンバ関数

	static HomingBullet *Spawn(Scene &scene, const Vector2 &position, const Vector2 &velocity, float turnRate, float homingTime);


public: // メンバ関数

	ProjectileKind GetKind() const override { return ProjectileKind::Homing; }


protected: // メンバ関数

	bool Move(float dt) override;


private: // メンバ変数

	// 1 秒あたりに曲がれる角度（ラジアン）
	float turnRate_ = 2.0f;
	float homingTime_ = 2.5f;
};

} // namespace ou
