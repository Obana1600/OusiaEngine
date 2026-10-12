#pragma once

#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Scene/Component.h"

// TODO: Behavior を継承させる

namespace ou {

class DirectionalLight: public Component {
public: // インナークラス

	// 定数バッファ: b3
	struct PerFrame {
		Vector4 color;
		Vector3 direction;
		float intensity;
	};


public: // メンバ関数

	void Awake() override;
	void OnDestroy() override;

	PerFrame MakePerFrame() const;

	void SetColor(const Vector4 &color);
	const Vector4 &GetColor() const;

	void SetIntensity(float intensity);
	float GetIntensity() const;


private: // メンバ変数

	Vector4 color_ = Vector4::One;
	float intensity_ = 1.0f;
};

} // namespace ou
