#pragma once

#include "Animation/Core/WrapMode.h"


namespace ou {

struct PlayheadStep {
	float previous = 0.0f;
	float next = 0.0f;

	bool includesPrevious = false;
	bool hasMoved = false;
	bool isFinished = false;
};

class AnimationPlayhead {
public: // メンバ関数

	void Restart();

	void Seek(float time);

	PlayheadStep Advance(float deltaTime, float length);

	float GetTime() const;

	void SetSpeed(float speed);
	float GetSpeed() const;

	void SetWrapMode(WrapMode wrapMode);
	WrapMode GetWrapMode() const;


private: // メンバ変数

	float time_ = 0.0f;
	float speed_ = 1.0f;
	WrapMode wrapMode_ = WrapMode::Once;
	bool isJustStarted_ = false;
	bool isReturning_ = false;
};

} // namespace ou
