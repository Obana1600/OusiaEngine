#pragma once

#include <limits>
#include <numbers>


namespace ou {

namespace Mathf {

inline constexpr float PI = std::numbers::pi_v<float>;

inline constexpr float Deg2Rad = PI / 180.0f;

inline constexpr float Rad2Deg = 180.0f / PI;

inline constexpr float Infinity = std::numeric_limits<float>::infinity();

inline constexpr float NegativeInfinity = -std::numeric_limits<float>::infinity();

inline constexpr float Epsilon = std::numeric_limits<float>::epsilon();


float Abs(float f);

float Acos(float f);

bool Approximately(float a, float b);

float Asin(float f);

float Atan2(float y, float x);

float Ceil(float f);

int CeilToInt(float f);

float Clamp(float value, float min, float max);

float Clamp01(float f);

float Cos(float f);

float Exp(float f);

float Floor(float f);

int FloorToInt(float f);

float InverseLerp(float a, float b, float t);

float Lerp(float a, float b, float t);

float LerpUnclamped(float a, float b, float t);

float Log(float f);

float Max(float a, float b);

float Min(float a, float b);

float Pow(float f, float p);

float Round(float f);

int RoundToInt(float f);

float Sign(float f);

int SignInt(float f);

float Sin(float f);

float Sqrt(float f);

float Tan(float f);

} // namespace Mathf

} // namespace ou
