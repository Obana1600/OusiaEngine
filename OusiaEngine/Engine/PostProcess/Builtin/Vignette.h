#pragma once

#include "PostProcess/VolumeComponent.h"


namespace ou {

class Vignette final: public VolumeComponent {
public: // メンバ変数

	ColorParameter color_{Color::Black};
	Vector2Parameter center_{Vector2(0.5f, 0.5f)};
	ClampedFloatParameter intensity_{0.0f, 0.0f, 1.0f};
	ClampedFloatParameter smoothness_{0.2f, 0.01f, 1.0f};
	BoolParameter rounded_{false};


public: // メンバ関数

	Vignette();

	bool IsActive() const;
};

} // namespace ou
