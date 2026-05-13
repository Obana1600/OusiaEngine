#pragma once

#include "Vector.h"
#include "Matrix.h"
#include <cstdint>


struct Material {
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};