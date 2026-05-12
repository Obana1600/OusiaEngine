#pragma once

#include "Vector.h"


struct DirectionLight {
	Vector4 color; // ライトの色
	Vector3 direction; // ライトの向き
	float intensity; // 輝度
};