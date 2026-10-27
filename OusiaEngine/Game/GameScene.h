#pragma once

#include "Scene/Component.h"
#include "Scene/Scene.h"


namespace ou {

// 前方宣言
class LineRenderer;

void BuildGameScene(Scene &scene);

// LineRenderer（壁で反射する予測線・予兆線）を試す
class LineRingTest final: public Component {
public: // メンバ関数

	void Start() override;
	void Update() override;


private: // メンバ変数

	LineRenderer *predictionLine_ = nullptr;
	LineRenderer *warningLine_ = nullptr;
	float logTimer_ = 0.0f;
};

} // namespace ou
