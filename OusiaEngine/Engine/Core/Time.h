#pragma once

#include <cstdint>


// TODO: 詳しく調べたい

namespace ou::Time {

float GetDeltaTime();

float GetUnscaledDeltaTime();

float GetTime();
double GetTimeAsDouble();

uint64_t GetFrameCount();

void SetTimeScale(float timeScale);
float GetTimeScale();

void Advance(float unscaledDeltaTime);

} // namespace ou::Time
