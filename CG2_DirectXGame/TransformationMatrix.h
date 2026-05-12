#pragma once

#include "Matrix.h"


struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
};