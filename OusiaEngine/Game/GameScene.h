#pragma once

#include "Scene/Component.h"
#include "Scene/Scene.h"


namespace ou {

class Rigidbody;

void BuildGameScene(Scene &scene);

// 単位 5（Rigidbody の補間）の確かめ
class InterpolationProbe final: public Component {
public:
	Rigidbody *none = nullptr;
	Rigidbody *interpolate = nullptr;
	Rigidbody *extrapolate = nullptr;
	Rigidbody *spinner = nullptr;
	Rigidbody *teleported = nullptr;
	Rigidbody *resting = nullptr;

	void Update() override;

private:
	int step_ = 0;

	// 0.5〜1.5 秒の、計算との差の最大
	float noneError_ = 0.0f;
	float interpolateError_ = 0.0f;
	float extrapolateError_ = 0.0f;
	float spinnerError_ = 0.0f;
	float interpolateFixedUpdateGap_ = 0.0f;

	// 0.5〜1.5 秒で、前のフレームから動いたフレームの数
	int frames_ = 0;
	int noneMoved_ = 0;
	int interpolateMoved_ = 0;
	float lastNoneX_ = 0.0f;
	float lastInterpolateX_ = 0.0f;

	// 瞬間移動のあと
	float teleportMinX_ = 1e9f;

	// 床で止まった玉
	float restingGap_ = 0.0f;
};

} // namespace ou
