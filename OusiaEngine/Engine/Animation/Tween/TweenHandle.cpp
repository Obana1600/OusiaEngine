#include "TweenHandle.h"
#include "Animation/Tween/TweenManager.h"

#include <algorithm>
#include <utility>

using namespace ou;


void ou::TweenHandle::Play() const {
	auto *tween = FindTween();
	if (tween && !tween->isCompleted) { tween->isPlaying = true; }
}


void ou::TweenHandle::Pause() const {
	if (auto *tween = FindTween()) { tween->isPlaying = false; }
}


void ou::TweenHandle::Restart() const {
	if (auto *tween = FindTween()) { tween->Restart(); }
}


void ou::TweenHandle::GoTo(float time, bool andPlay) const {
	auto *tween = FindTween();
	if (!tween) { return; }

	tween->Seek(time);
	tween->isPlaying = andPlay;
}


void ou::TweenHandle::Complete() const {
	auto *tween = FindTween();
	if (!tween || tween->isCompleted || tween->IsInfinite()) { return; }

	tween->Seek(tween->CalculateTotalDuration());

	if (!FindTween()) { return; }
	TweenManager::Finish(index_);
}


void ou::TweenHandle::Kill() const {
	if (FindTween()) { TweenManager::Kill(index_); }
}


TweenHandle ou::TweenHandle::SetEase(EaseType easeType) const {
	if (auto *tween = FindTween()) { tween->easeType = easeType; }
	return *this;
}


TweenHandle ou::TweenHandle::SetLoops(int32_t loopCount, TweenLoopType loopType) const {
	auto *tween = FindTween();
	if (!tween || tween->isStarted) { return *this; }

	tween->loopCount = loopCount == 0 ? 1 : loopCount;
	tween->loopType = loopType;
	return *this;
}


TweenHandle ou::TweenHandle::SetDelay(float delay) const {
	auto *tween = FindTween();
	if (!tween || tween->isStarted) { return *this; }

	tween->delay = Mathf::Max(delay, 0.0f);
	tween->delayRemaining = tween->delay;
	return *this;
}


TweenHandle ou::TweenHandle::SetUnscaledTime(bool usesUnscaledTime) const {
	if (auto *tween = FindTween()) { tween->usesUnscaledTime = usesUnscaledTime; }
	return *this;
}


TweenHandle ou::TweenHandle::SetAutoKill(bool autoKill) const {
	if (auto *tween = FindTween()) { tween->autoKill = autoKill; }
	return *this;
}


TweenHandle ou::TweenHandle::OnComplete(std::function<void()> onComplete) const {
	if (auto *tween = FindTween()) { tween->onComplete = std::move(onComplete); }
	return *this;
}


bool ou::TweenHandle::IsValid() const {
	return FindTween() != nullptr;
}


bool ou::TweenHandle::IsPlaying() const {
	const auto *tween = FindTween();
	return tween && tween->isPlaying;
}


ou::TweenHandle::TweenHandle(uint32_t index, uint32_t generation)
	: index_(index),
	  generation_(generation) {}


Tween *ou::TweenHandle::FindTween() const {
	return TweenManager::FindTween(index_, generation_);
}
