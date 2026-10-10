#include "Mathf.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>


float ou::Mathf::Abs(float f) {
	return std::abs(f);
}


float ou::Mathf::Acos(float f) {
	return std::acos(f);
}


bool ou::Mathf::Approximately(float a, float b) {
	if (a == b) {
		return true;
	}
	const auto scale = Max(Abs(a), Abs(b));
	return Abs(a - b) <= Max(scale * 1e-6f, 1e-6f);
}


float ou::Mathf::Asin(float f) {
	return std::asin(f);
}


float ou::Mathf::Atan2(float y, float x) {
	return std::atan2(y, x);
}


float ou::Mathf::Ceil(float f) {
	return std::ceil(f);
}


int ou::Mathf::CeilToInt(float f) {
	return static_cast<int>(std::ceil(f));
}


float ou::Mathf::Clamp(float value, float min, float max) {
	return std::clamp(value, min, max);
}


float ou::Mathf::Clamp01(float f) {
	return std::clamp(f, 0.0f, 1.0f);
}


float ou::Mathf::Cos(float f) {
	return std::cos(f);
}


float ou::Mathf::Exp(float f) {
	return std::exp(f);
}


float ou::Mathf::Floor(float f) {
	return std::floor(f);
}


int ou::Mathf::FloorToInt(float f) {
	return static_cast<int>(std::floor(f));
}


float ou::Mathf::InverseLerp(float a, float b, float t) {
	return Clamp01((t - a) / (b - a));
}


float ou::Mathf::Lerp(float a, float b, float t) {
	return a + (b - a) * Clamp01(t);
}


float ou::Mathf::LerpUnclamped(float a, float b, float t) {
	return a + (b - a) * t;
}


float ou::Mathf::Log(float f) {
	return std::log(f);
}


float ou::Mathf::Max(float a, float b) {
	return std::max(a, b);
}


float ou::Mathf::Min(float a, float b) {
	return std::min(a, b);
}


float ou::Mathf::Pow(float f, float p) {
	return std::pow(f, p);
}


float ou::Mathf::Round(float f) {
	return std::round(f);
}


int ou::Mathf::RoundToInt(float f) {
	return static_cast<int>(std::round(f));
}


float ou::Mathf::Sign(float f) {
	return f >= 0.0f ? 1.0f : -1.0f;
}


int ou::Mathf::SignInt(float f) {
	return f >= 0.0f ? 1 : -1;
}


float ou::Mathf::Sin(float f) {
	return std::sin(f);
}


float ou::Mathf::Sqrt(float f) {
	return std::sqrt(f);
}


float ou::Mathf::Tan(float f) {
	return std::tan(f);
}
