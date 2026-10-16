#pragma once

#include "Math/Vector2.h"
#include "Math/Vector3.h"

#include <cstdint>

// TODO: 詳しく調べる

namespace ou::Random {

// 乱数列を初期化する
void InitState(uint32_t seed);

// 0.0f 以上 1.0f 未満
float Value();

// min 以上 max 未満
float Range(float min, float max);

// min 以上 max 未満
int Range(int min, int max);

// 半径 1 の球の中の点
Vector3 InsideUnitSphere();

// 半径 1 の球の表面の点
Vector3 OnUnitSphere();

// 半径 1 の円の中の点
Vector2 InsideUnitCircle();

} // namespace ou::Random
