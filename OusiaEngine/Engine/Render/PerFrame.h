#pragma once

#include "Math/Vector4.h"
#include "Render/Light.h"

#include <cstdint>


namespace ou {

struct PerFrame {
	Vector4 time;
	uint32_t lightCount;
	uint32_t padding[3];
	Light::ShaderLight lights[Light::kMaxLights];
};

} // namespace ou
