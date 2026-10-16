#include "ParticleSystem.h"
#include "Core/Time.h"
#include "Math/Mathf.h"
#include "Math/Random.h"
#include "Scene/Transform.h"

#include <utility>

using namespace ou;


namespace {

constexpr float kGravity = -9.81f;

}



float ou::FloatRange::Evaluate() const {
	if (min == max) { return min; }
	return (min < max) ? Random::Range(min, max) : Random::Range(max, min);
}


Color ou::ColorRange::Evaluate() const {
	const auto t = Random::Value();
	return Color::Lerp(min, max, t);
}
