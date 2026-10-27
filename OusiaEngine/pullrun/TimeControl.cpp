#include "TimeControl.h"
#include "Core/Time.h"
#include "Tuning.h"

#include <algorithm>

using namespace ou;



void ou::TimeControl::RequestHitStop(float seconds) {
	sHitStopRemaining_ = std::max(sHitStopRemaining_, seconds);
}


void ou::TimeControl::SetSlowMotion(bool isSlow) {
	sIsSlow_ = isSlow;
}


void ou::TimeControl::Awake() {
	sHitStopRemaining_ = 0.0f;
	sIsSlow_ = false;
	Time::SetTimeScale(1.0f);
}


void ou::TimeControl::Update() {
	// ヒットストップは実時間で数える
	sHitStopRemaining_ = std::max(sHitStopRemaining_ - Time::GetUnscaledDeltaTime(), 0.0f);

	if (sHitStopRemaining_ > 0.0f) {
		Time::SetTimeScale(0.0f);
	} else if (sIsSlow_) {
		Time::SetTimeScale(Tuning::kChargeTimeScale);
	} else {
		Time::SetTimeScale(1.0f);
	}
}


void ou::TimeControl::OnDestroy() {
	Time::SetTimeScale(1.0f);
}
