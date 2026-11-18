#include "Time.h"
#include "Math/Mathf.h"

using namespace ou;


namespace {

constexpr float kMaxDeltaTime = 1.0f / 3.0f;

constexpr float kMinFixedDeltaTime = 0.0001f;

} // namespace



float ou::Time::GetDeltaTime() {
	return sIsInFixedTimeStep_ ? sFixedDeltaTime_ : sDeltaTime_;
}


float ou::Time::GetUnscaledDeltaTime() {
	return sUnscaledDeltaTime_;
}


float ou::Time::GetTime() {
	return static_cast<float>(GetTimeAsDouble());
}


double ou::Time::GetTimeAsDouble() {
	return sIsInFixedTimeStep_ ? sFixedTime_ : sTime_;
}


uint64_t ou::Time::GetFrameCount() {
	return sFrameCount_;
}


void ou::Time::SetTimeScale(float timeScale) {
	sTimeScale_ = timeScale;
}


float ou::Time::GetTimeScale() {
	return sTimeScale_;
}


void ou::Time::SetFixedDeltaTime(float fixedDeltaTime) {
	sFixedDeltaTime_ = Mathf::Max(fixedDeltaTime, kMinFixedDeltaTime);
}


float ou::Time::GetFixedDeltaTime() {
	return sFixedDeltaTime_;
}


float ou::Time::GetFixedTime() {
	return static_cast<float>(sFixedTime_);
}


double ou::Time::GetFixedTimeAsDouble() {
	return sFixedTime_;
}


bool ou::Time::IsInFixedTimeStep() {
	return sIsInFixedTimeStep_;
}


void ou::Time::Advance(float unscaledDeltaTime) {
	sUnscaledDeltaTime_ = Mathf::Min(unscaledDeltaTime, kMaxDeltaTime);
	sDeltaTime_ = sUnscaledDeltaTime_ * sTimeScale_;
	sTime_ += sDeltaTime_;
	sFrameCount_++;
}


void ou::Time::Freeze() {
	sUnscaledDeltaTime_ = 0.0f;
	sDeltaTime_ = 0.0f;
}


bool ou::Time::AdvanceFixedTime() {
	sIsInFixedTimeStep_ = (sFixedTime_ + sFixedDeltaTime_ <= sTime_);
	if (sIsInFixedTimeStep_) {
		sFixedTime_ += sFixedDeltaTime_;
	}
	return sIsInFixedTimeStep_;
}


void ou::Time::ResetForPlayMode() {
	sTime_ = 0.0;
	sFixedTime_ = 0.0;
	sTimeScale_ = 1.0f;
}