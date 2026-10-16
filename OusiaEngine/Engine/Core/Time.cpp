#include "Time.h"
#include "Math/Mathf.h"

using namespace ou;


namespace {

constexpr float kMaxDeltaTime = 1.0f / 3.0f;

float sDeltaTime = 0.0f;
float sUnscaledDeltaTime = 0.0f;
double sTime = 0.0;
uint64_t sFrameCount = 0;
float sTimeScale = 1.0f;

} // namespace



float ou::Time::GetDeltaTime() {
	return	sDeltaTime;
}


float ou::Time::GetUnscaledDeltaTime() {
	return sUnscaledDeltaTime;
}


float ou::Time::GetTime() {
	return static_cast<float>(sTime);
}


double ou::Time::GetTimeAsDouble() {
	return sTime;
}


uint64_t ou::Time::GetFrameCount() {
	return sFrameCount;
}


void ou::Time::SetTimeScale(float timeScale) {
	sTimeScale = timeScale;
}


float ou::Time::GetTimeScale() {
	return sTimeScale;
}


void ou::Time::Advance(float unscaledDeltaTime) {
	sUnscaledDeltaTime = Mathf::Min(unscaledDeltaTime, kMaxDeltaTime);
	sDeltaTime = sUnscaledDeltaTime * sTimeScale;
	sTime += sDeltaTime;
	sFrameCount++;
}
