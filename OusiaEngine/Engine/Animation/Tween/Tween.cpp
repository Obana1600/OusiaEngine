#include "Tween.h"
#include "Math/Mathf.h"

#include <algorithm>

using namespace ou;


bool ou::Tween::Advance(float deltaTime) {
	auto remaining = deltaTime;

	if (delayRemaining > 0.0f) {
		delayRemaining -= remaining;
		if (delayRemaining > 0.0f) { return false; }

		remaining = -delayRemaining;
		delayRemaining = 0.0f;
	}

	StartOnce();

	position += remaining;
	ApplyAt(position);

	if (HasReachedEnd()) { return true; }

	if (IsInfinite()) { position = Mathf::Repeat(position, duration * 2.0f); }

	return false;
}


void ou::Tween::Seek(float time) {
	delayRemaining = 0.0f;
	isCompleted = false;
	StartOnce();

	if (duration <= 0.0f) {
		position = 0.0f;
	} else if(IsInfinite()) {
		position = Mathf::Repeat(Mathf::Max(time, 0.0f), duration * 2.0f);
	} else {
		position = Mathf::Clamp(time, 0.0f, CalculateTotalDuration());
	}

	ApplyAt(position);
}


void ou::Tween::Restart() {
	delayRemaining = delay;
	position = 0.0f;
	isPlaying = true;
	isCompleted = false;

	if (isStarted) { ApplyAt(0.0f); }
}


bool ou::Tween::IsInfinite() const {
	return loopCount < 0;
}


float ou::Tween::CalculateTotalDuration() const {
	return duration * static_cast<float>(loopCount);
}


void ou::Tween::StartOnce() {
	if (isStarted) { return; }

	captureStartValue();
	isStarted = true;
}


bool ou::Tween::HasReachedEnd() const {
	return duration <= 0.0f || (!IsInfinite() && position >= CalculateTotalDuration());
}


void ou::Tween::ApplyAt(float time) const {
	if (duration <= 0.0f) {
		applyValue(1.0f);
		return;
	}

	auto loop = static_cast<int64_t>(time / duration);
	auto loopTime = time - static_cast<float>(loop) * duration;
	if (!IsInfinite() && loop >= loopCount) {
		loop = static_cast<int64_t>(loopCount) - 1;
		loopTime = duration;
	}

	auto ratio = loopTime / duration;
	if (loopType == TweenLoopType::Yoyo && loop % 2 == 1) { ratio = 1.0f - ratio; }

	applyValue(Easing::Evaluate(easeType, ratio));
}
