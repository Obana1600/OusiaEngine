#pragma once

#include "PostProcess/VolumeComponent.h"


namespace ou {

class ColorAdjustments final: public VolumeComponent {
public: // メンバ変数

	FloatParameter postExposure_{0.0f};
	ClampedFloatParameter contrast_{0.0f, -100.0f, 100.0f};
	ColorParameter colorFilter_{Color::White};
	ClampedFloatParameter hueShift_{0.0f, -180.0f, 180.0f};
	ClampedFloatParameter saturation_{0.0f, -100.0f, 100.0f};


public: // メンバ関数

	ColorAdjustments();

	bool IsActive() const;
};

} // namespace ou
