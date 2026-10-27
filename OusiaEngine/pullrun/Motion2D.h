#pragma once

#include "Math/Vector2.h"

#include <vector>


namespace ou {

// xy 平面を動く球の移動と、アリーナの壁での反射。
// プレイヤー・巨大弾・突進中のボスと、予測線・予兆線の計算で同じものを使い、実際の軌道とのずれを防ぐ
struct MotionParams {
	float radius = 0.5f;
	// 1 秒で exp(-drag) 倍に減速する
	float drag = 0.0f;
	// 壁で反射したときに残る速さの割合
	float restitution = 1.0f;
	// -y 向きの重力。0 で重力なし
	float gravity = 0.0f;
};

struct MotionState {
	Vector2 position;
	Vector2 velocity;
};

struct MotionStepResult {
	int bounceCount = 0;
	// 最後に当たった壁の法線（アリーナの内向き）
	Vector2 lastWallNormal;
	// 壁で反射したことによる速度の変化（質量 1 の力積）の合計
	Vector2 wallImpulse;
};

namespace Motion2D {

// dt だけ進める。速いときはサブステップに分けて、壁のすり抜けを防ぐ
MotionStepResult Step(MotionState &state, const MotionParams &params, float dt);

// state から stepTime ごとに進めた位置の列（予測線・予兆線）。
// maxBounces 回反射するか、速さが stopBelowSpeed 未満になったら止める
std::vector<Vector2> Trace(MotionState state, const MotionParams &params, float duration, float stepTime,
						   int maxBounces, float stopBelowSpeed = 0.0f);

Vector2 Reflect(const Vector2 &velocity, const Vector2 &normal);

} // namespace Motion2D

} // namespace ou
