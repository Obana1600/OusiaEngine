#pragma once

#include "Math/Vector2.h"
#include "Scene/Component.h"


namespace ou {

// 前方宣言
class RectTransform;

// タイトル画面：中央上にロゴ、中央下に「プレイ」「終了」、その下に揺れる「Click To Start」
class TitleMenu final: public Component {
public: // メンバ関数

	void Start() override;
	void Update() override;


private: // メンバ変数

	RectTransform *logo_ = nullptr;
	Vector2 logoPosition_;
	RectTransform *prompt_ = nullptr;
	float time_ = 0.0f;
};

} // namespace ou
