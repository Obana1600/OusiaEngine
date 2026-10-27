#pragma once

#include "Math/Color.h"
#include "Math/Vector2.h"
#include "Scene/Component.h"


namespace ou {

// 前方宣言
class Boss;
class Player;
class Projectile;

// プレイヤー・ボス・弾の当たり判定と、その結果（ダメージ・反射・ヒットストップ・コンボ）。
// 移動が終わったあとに判定するため、移動するコンポーネントより後に作る
class CombatSystem final: public Component {
public: // メンバ関数

	void Update() override;


private: // メンバ関数

	static void ResolvePlayerBoss(Player &player, Boss &boss);
	static void ResolvePlayerProjectile(Player &player, Projectile &projectile);
	// コンボの加算・ヒットストップ・揺れ・はじけるパーティクル。position と direction ははじける位置と向き
	static void OnScore(float speed, const Vector2 &position, const Vector2 &direction, const Color &color);
};

} // namespace ou
