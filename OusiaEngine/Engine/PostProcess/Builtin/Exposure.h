#pragma once

#include "PostProcess/VolumeComponent.h"


namespace ou {

// TODO: 追加する
enum class ExposureMode {
	Fixed = 0,
	AutomaticHistogram = 4,
};

enum class AdaptationMode {
	Fixed = 0,
	Progressive = 1,
};


using ExposureModeParameter = VolumeParameter<ExposureMode>;
using AdaptationModeParameter = VolumeParameter<AdaptationMode>;


class Exposure final: public VolumeComponent {
public: // メンバ変数

	ExposureModeParameter mode_{ExposureMode::Fixed};
	FloatParameter fixedExposure_{0.0f};
	FloatParameter compensation_{0.0f};

	// 自動モードで使う
	FloatParameter limitMin_{-1.0f};
	FloatParameter limitMax_{14.0f};
	AdaptationModeParameter adaptationMode_{AdaptationMode::Progressive};
	MinFloatParameter adaptationSpeedDarkToLight_{3.0f, 0.001f};
	MinFloatParameter adaptationSpeedLightToDark_{1.0f, 0.001f};
	Vector2Parameter histogramPercentages_{{40.0f, 90.0f}};


public: // メンバ関数

	Exposure();

	bool IsActive() const;
};

} // namespace ou
