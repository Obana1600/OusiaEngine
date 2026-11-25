#pragma once

#include "PostProcess/VolumeComponent.h"


namespace ou {

class Bloom final: public VolumeComponent {
public: // メンバ変数

	MinFloatParameter threshold_{0.9f, 0.0f};
	MinFloatParameter intensity_{0.0f, 0.0f};
	ClampedFloatParameter scatter_{0.7f, 0.0f, 1.0f};
	MinFloatParameter clamp_{65472.0f, 0.0f};
	ColorParameter tint_{Color::White};


public: // メンバ関数

	Bloom();

	bool IsActive() const;
};

} // namespace ou
