#include "Random.h"
#include "Math/Mathf.h"

#include <random>

using namespace ou;


namespace {

std::mt19937 &Engine() {
	static std::mt19937 engine{std::random_device{}()};
	return engine;
}

} // namespace



void ou::Random::InitState(uint32_t seed) {
	Engine().seed(seed);
}


float ou::Random::Value() {
	return std::uniform_real_distribution<float>(0.0f, 1.0f)(Engine());
}


float ou::Random::Range(float min, float max) {
	return std::uniform_real_distribution<float>(min, max)(Engine());
}


int ou::Random::Range(int minInclusive, int maxExclusive) {
	if (maxExclusive <= minInclusive) { return minInclusive; }
	return std::uniform_int_distribution<int>(minInclusive, maxExclusive - 1)(Engine());
}


Vector3 ou::Random::InsideUnitSphere() {
	while (true) {
		const Vector3 p = {Range(-1.0f, 1.0f),
						   Range(-1.0f, 1.0f),
						   Range(-1.0f, 1.0f)};
		if (p.LengthSquared() <= 1.0f) { return p; }
	}
}


Vector3 ou::Random::OnUnitSphere() {
	while (true) {
		const auto p = InsideUnitSphere();
		const auto length2 = p.LengthSquared();
		if (length2 > Mathf::Epsilon) { return p / Mathf::Sqrt(length2); }
	}
}


Vector2 ou::Random::InsideUnitCircle() {
	while (true) {
		const Vector2 p = {Range(-1.0f, 1.0f),
						   Range(-1.0f, 1.0f)};
		if (p.LengthSquared() <= 1.0f) { return p; }
	}
}
