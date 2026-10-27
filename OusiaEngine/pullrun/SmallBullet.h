#pragma once

#include "Projectile.h"


namespace ou {

// 前方宣言
class Scene;

// まっすぐ飛び、アリーナの壁で消える小弾
class SmallBullet final: public Projectile {
public: // 静的メンバ関数

	static SmallBullet *Spawn(Scene &scene, const Vector2 &position, const Vector2 &velocity);


public: // メンバ関数

	ProjectileKind GetKind() const override { return ProjectileKind::Small; }


protected: // メンバ関数

	bool Move(float dt) override;
};

} // namespace ou
