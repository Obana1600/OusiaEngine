#pragma once

#include "Vector.h"
#include <cstdint>


struct Material {
	Vector4 color;
	int32_t enableLighting;
};