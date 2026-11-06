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

void SetFixedDeltaTime(float fixedDeltaTime);
float GetFixedDeltaTime();

float GetFixedTime();
double GetFixedTimeAsDouble();
bool IsInFixedTimeStep();

void Advance(float unscaledDeltaTime);
bool AdvanceFixedTime();

} // namespace ou::Time
