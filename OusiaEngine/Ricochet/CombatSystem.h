#pragma once

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
	static void OnScore(float speed);
};

} // namespace ou
