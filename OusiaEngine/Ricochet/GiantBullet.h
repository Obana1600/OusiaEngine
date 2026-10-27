#pragma once

#include "Motion2D.h"
#include "Projectile.h"


namespace ou {

// 前方宣言
class Scene;

// アリーナの壁で反射しながら飛ぶ巨大弾。決まった回数反射すると消える。
// 高速のプレイヤーが当たると壊れる（CombatSystem）
class GiantBullet final: public Projectile {
public: // 静的メンバ関数

	static GiantBullet *Spawn(Scene &scene, const Vector2 &position, const Vector2 &velocity, float radius);


public: // メンバ関数

	ProjectileKind GetKind() const override { return ProjectileKind::Giant; }


protected: // メンバ関数

	bool Move(float dt) override;


private: // メンバ変数

	int bounceCount_ = 0;
};

} // namespace ou
