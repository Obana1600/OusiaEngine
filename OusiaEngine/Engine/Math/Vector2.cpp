#include "Vector2.h"
#include "Diagnostics/Assert.h"
#include "Math/Mathf.h"
#include "Math/Vector3.h"

#include <format>

using namespace ou;


const Vector2 ou::Vector2::Zero = {0.0f, 0.0f};
const Vector2 ou::Vector2::One = {1.0f, 1.0f};
const Vector2 ou::Vector2::Up = {0.0f, 1.0f};
const Vector2 ou::Vector2::Down = {0.0f, -1.0f};
const Vector2 ou::Vector2::Left = {-1.0f, 0.0f};
const Vector2 ou::Vector2::Right = {1.0f, 0.0f};
const Vector2 ou::Vector2::PositiveInfinity = {Mathf::Infinity, Mathf::Infinity};
const Vector2 ou::Vector2::NegativeInfinity = {Mathf::NegativeInfinity, Mathf::NegativeInfinity};


ou::Vector2::Vector2()
	: x(0.0f),
	  y(0.0f) {}


ou::Vector2::Vector2(float x, float y)
	: x(x),
	  y(y) {}


bool ou::Vector2::Equals(const Vector2 &other) const {
	return x == other.x &&
		   y == other.y;
}


float ou::Vector2::Length() const {
	return Mathf::Sqrt(LengthSquared());
}


float ou::Vector2::LengthSquared() const {
	return (x * x) + (y * y);
}


Vector2 ou::Vector2::Normalized() const {
	float len = Length();
	if (len == 0.0f) { return Vector2::Zero; }
	return {x / len, y / len};
}


std::string ou::Vector2::ToString() const {
	return std::format("({:.2f}, {:.2f})", x, y);
}


std::string ou::Vector2::ToString(std::string_view format) const {
	const auto fmt = std::format("({{:{0}}}, {{:{0}}})", format);
	return std::vformat(fmt, std::make_format_args(x, y));
}


Vector2 ou::Vector2::Normalize(Vector2 &v) {
	return v = v.Normalized();
}


float ou::Vector2::Distance(const Vector2 &v1, const Vector2 &v2) {
	return (v2 - v1).Length();
}


float ou::Vector2::Dot(const Vector2 &v1, const Vector2 &v2) {
	return (v1.x * v2.x) + (v1.y * v2.y);
}


float ou::Vector2::Cross(const Vector2 &v1, const Vector2 &v2) {
	return (v1.x * v2.y) - (v1.y * v2.x);
}


Vector2 ou::Vector2::Min(const Vector2 &v1, const Vector2 &v2) {
	return {Mathf::Min(v1.x, v2.x),
			Mathf::Min(v1.y, v2.y)};
}


Vector2 ou::Vector2::Max(const Vector2 &v1, const Vector2 &v2) {
	return {Mathf::Max(v1.x, v2.x),
			Mathf::Max(v1.y, v2.y)};
}


Vector2 ou::Vector2::Scale(const Vector2 &v1, const Vector2 &v2) {
	return {v1.x * v2.x,
			v1.y * v2.y};
}


Vector2 ou::Vector2::Lerp(const Vector2 &a, const Vector2 &b, float t) {
	t = Mathf::Clamp01(t);
	return LerpUnclamped(a, b, t);
}


Vector2 ou::Vector2::LerpUnclamped(const Vector2 &a, const Vector2 &b, float t) {
	return a + (b - a) * t;
}


namespace ou {

Vector2 operator+(const Vector2 &v1, const Vector2 &v2) {
	return {v1.x + v2.x,
			v1.y + v2.y};
}


Vector2 operator-(const Vector2 &v1, const Vector2 &v2) {
	return {v1.x - v2.x,
			v1.y - v2.y};
}


Vector2 operator*(const Vector2 &v1, float v2) {
	return {v1.x * v2,
			v1.y * v2};
}


Vector2 operator*(float v1, const Vector2 &v2) {
	return v2 * v1;
}


Vector2 operator/(const Vector2 &v1, float v2) {
	return {v1.x / v2,
			v1.y / v2};
}


Vector2 operator/(float v1, const Vector2 &v2) {
	return {v1 / v2.x,
			v1 / v2.y};
}


bool operator==(const Vector2 &v1, const Vector2 &v2) {
	return Mathf::Approximately(v1.x, v2.x) &&
		   Mathf::Approximately(v1.y, v2.y);
}

} // namespace ou


Vector2 &ou::Vector2::operator+=(const Vector2 &v) {
	return *this = *this + v;
}


Vector2 &ou::Vector2::operator-=(const Vector2 &v) {
	return *this = *this - v;
}


Vector2 &ou::Vector2::operator*=(float v) {
	return *this = *this * v;
}


Vector2 &ou::Vector2::operator/=(float v) {
	return *this = *this / v;
}


float &ou::Vector2::operator[](int index) {
	switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		default:
			ASSERT(false, "範囲外のインデックスです: {}", index);
			return x;
	}
}


float ou::Vector2::operator[](int index) const {
	switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		default:
			ASSERT(false, "範囲外のインデックスです: {}", index);
			return x;
	}
}


Vector2 ou::Vector2::operator-() const {
	return {-x, -y};
}


ou::Vector2::operator Vector3() const {
	return {x, y, 0.0f};
}
