#pragma once

#include "Math/Color.h"


namespace ou {

struct ColorBlock {
	Color normalColor = Color::White;
	Color highlightedColor = {0.9607843f, 0.9607843f, 0.9607843f, 1.0f};
	Color pressedColor = {0.7843137f, 0.7843137f, 0.7843137f, 1.0f};
	Color selectedColor = {0.9607843f, 0.9607843f, 0.9607843f, 1.0f};
	Color disabledColor = {0.7843137f, 0.7843137f, 0.7843137f, 0.5019608f};
	float colorMultiplier = 1.0f;
	float fadeDuration = 0.1f;
};

} // namespace ou
