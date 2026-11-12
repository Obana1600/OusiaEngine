#pragma once

#include "Animation/Core/Easing.h"
#include "Animation/Tween/Tween.h"

#include <cstdint>
#include <functional>


namespace ou {

class TweenHandle {
	friend class TweenManager;

public: // メンバ関数

	TweenHandle() = default;

	void Play() const;
	void Pause() const;
	void Restart() const;
	void GoTo(float time, bool andPlay = false) const;
	void Complete() const;
	void Kill() const;

	TweenHandle SetEase(EaseType easeType) const;
	TweenHandle SetLoops(int32_t loopCount, TweenLoopType loopType = TweenLoopType::Restart) const;
	TweenHandle SetDelay(float delay) const;
	TweenHandle SetUnscaledTime(bool usesUnscaledTime) const;
	TweenHandle SetAutoKill(bool autoKill) const;

	TweenHandle OnComplete(std::function<void()> onComplete) const;

	bool IsValid() const;
	bool IsPlaying() const;


private: // メンバ変数

	uint32_t index_ = 0;
	uint32_t generation_ = 0;


private: // メンバ関数

	TweenHandle(uint32_t index, uint32_t generation);

	Tween *FindTween() const;
};

} // namespace ou
