#pragma once

#include "Animation/Core/Easing.h"

#include <cstdint>
#include <functional>


namespace ou {

enum class TweenLoopType {
	Restart,
	Yoyo,
	Incremental,
};

struct Tween {
public: // メンバ変数

	uint32_t ownerId = 0;

	std::function<void()> captureStartValue;
	std::function<void(float)> applyValue;
	std::function<void()> onComplete;

	float duration = 0.0f;
	float delay = 0.0f;
	EaseType easeType = EaseType::OutQuad;

	int32_t loopCount = 1;
	TweenLoopType loopType = TweenLoopType::Restart;

	bool usesUnscaledTime = false;
	bool autoKill = true;

	float delayRemaining = 0.0f;
	float position = 0.0f;
	bool isStarted = false;
	bool isPlaying = true;
	bool isCompleted = false;


public: // メンバ関数

	bool Advance(float deltaTime);

	void Seek(float time);

	void Restart();

	bool IsInfinite() const;
	float CalculateTotalDuration() const;


private: // メンバ関数

	void StartOnce();
	bool HasReachedEnd() const;
	void ApplyAt(float time) const;
};

} // namespace ou
