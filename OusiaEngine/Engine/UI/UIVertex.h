#pragma once

#include "Math/Color.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"


namespace ou {

struct UIVertex {
	Vector3 position;
	Color color = Color::White;
	Vector4 uv0;
	Vector4 uv1;
};

} // namespace ou
