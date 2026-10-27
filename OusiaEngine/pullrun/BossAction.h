#pragma once

#include "Math/Vector2.h"


namespace ou {

// 前方宣言
class Boss;

// ボスからプレイヤーへの向き（プレイヤーがいなければ左向き）
Vector2 DirectionToPlayer(const Boss &boss);
// 向き direction から angle（ラジアン）だけ回した単位ベクトル
Vector2 RotateDirection(const Vector2 &direction, float angle);

// ボスの行動 1 つ。Boss が選んで Begin → Update（true で終わり）→ End の順に呼ぶ。
// 第二形態への移行などで途中で打ち切られても End は呼ばれるので、予兆の表示などは End で片付ける
class BossAction {
public: // メンバ関数

	virtual ~BossAction() = default;

	virtual void Begin(Boss &) {}
	// 終わったら true
	virtual bool Update(Boss &boss, float dt) = 0;
	virtual void End(Boss &) {}
};

} // namespace ou
