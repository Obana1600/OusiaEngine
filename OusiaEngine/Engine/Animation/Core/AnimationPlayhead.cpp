#include "AnimationPlayhead.h"
#include "Math/Mathf.h"

using namespace ou;


namespace {

float CalculateCycle(WrapMode wrapMode, float length) {
	if (length <= 0.0f) { return 0.0f; }
	if (wrapMode == WrapMode::Loop) { return length; }
	if (wrapMode == WrapMode::PingPong) { return length * 2.0f; }
	return 0.0f;
}

} // namespace



void ou::AnimationPlayhead::Restart() {
	time_ = 0.0f;
	isJustStarted_ = true;
	isReturning_ = false;
}


void ou::AnimationPlayhead::Seek(float time) {
	time_ = time;
	isReturning_ = false;
}


PlayheadStep ou::AnimationPlayhead::Advance(float deltaTime, float length) {
	const auto delta = deltaTime * speed_;
	const auto cycle = CalculateCycle(wrapMode_, length);

	PlayheadStep step;

	if (cycle > 0.0f) {
		step.previous = Mathf::Repeat(isReturning_ ? cycle - time_ : time_, cycle);
		step.next = step.previous + delta;
		const auto position = Mathf::Repeat(step.next, cycle);
		isReturning_ = position > length;
		time_ = isReturning_ ? cycle - position : position;
	} else {
		step.previous = Mathf::Clamp(time_, 0.0f, length);
		step.next = Mathf::Clamp(step.previous + delta, 0.0f, length);
		time_ = step.next;
	}

	step.isFinished = cycle <= 0.0f &&
					  wrapMode_ != WrapMode::ClampForever &&
					  (length <= 0.0f ||
					   (speed_ > 0.0f && time_ >= length) ||
					   (speed_ < 0.0f && time_ <= 0.0f));

	step.hasMoved = delta != 0.0f || step.isFinished;
	if (step.hasMoved) {
		step.includesPrevious = isJustStarted_;
		isJustStarted_ = false;
	}

	return step;
}


float ou::AnimationPlayhead::GetTime() const {
	return time_;
}


void ou::AnimationPlayhead::SetSpeed(float speed) {
	speed_ = speed;
}


float ou::AnimationPlayhead::GetSpeed() const {
	return speed_;
}


void ou::AnimationPlayhead::SetWrapMode(WrapMode wrapMode) {
	wrapMode_ = wrapMode;
	isReturning_ = false;
}


WrapMode ou::AnimationPlayhead::GetWrapMode() const {
	return wrapMode_;
}
