#pragma once

#include "PostProcess/VolumeComponent.h"


namespace ou {

enum class TonemappingMode {
	None,
	Neutral,
	ACES,
};

using TonemappingModeParameter = VolumeParameter<TonemappingMode>;

class Tonemapping final: public VolumeComponent {
public: // メンバ変数

	TonemappingModeParameter mode_{TonemappingMode::None};


public: // メンバ関数

	Tonemapping();

	bool IsActive() const;
};

} // namespace ou
