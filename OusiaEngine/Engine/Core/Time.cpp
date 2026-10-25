#include "Time.h"
#include "Math/Mathf.h"

using namespace ou;


namespace {

constexpr float kMaxDeltaTime = 1.0f / 3.0f;

constexpr float kMinFixedDeltaTime = 0.0001f;

float sDeltaTime = 0.0f;
float sUnscaledDeltaTime = 0.0f;
double sTime = 0.0;
uint64_t sFrameCount = 0;
float sTimeScale = 1.0f;

float sFixedDeltaTime = 0.02f;
double sFixedTime = 0.0;
bool sIsInFixedTimeStep = false;

} // namespace



float ou::Time::GetDeltaTime() {
	return sIsInFixedTimeStep ? sFixedDeltaTime : sDeltaTime;
}


float ou::Time::GetUnscaledDeltaTime() {
	return sUnscaledDeltaTime;
}


float ou::Time::GetTime() {
	return static_cast<float>(GetTimeAsDouble());
}


double ou::Time::GetTimeAsDouble() {
	return sIsInFixedTimeStep ? sFixedTime : sTime;
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


void ou::Time::SetFixedDeltaTime(float fixedDeltaTime) {
	sFixedDeltaTime = Mathf::Max(fixedDeltaTime, kMinFixedDeltaTime);
}


float ou::Time::GetFixedDeltaTime() {
	return sFixedDeltaTime;
}


float ou::Time::GetFixedTime() {
	return static_cast<float>(sFixedTime);
}


bool ou::Time::IsInFixedTimeStep() {
	return sIsInFixedTimeStep;
}


void ou::Time::Advance(float unscaledDeltaTime) {
	sUnscaledDeltaTime = Mathf::Min(unscaledDeltaTime, kMaxDeltaTime);
	sDeltaTime = sUnscaledDeltaTime * sTimeScale;
	sTime += sDeltaTime;
	sFrameCount++;
}


bool ou::Time::AdvanceFixedTime() {
	sIsInFixedTimeStep = (sFixedTime + sFixedDeltaTime <= sTime);
	if (sIsInFixedTimeStep) {
		sFixedTime += sFixedDeltaTime;
	}
	return sIsInFixedTimeStep;
}
