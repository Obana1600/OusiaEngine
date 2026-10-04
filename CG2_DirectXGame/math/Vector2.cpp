#include "Vector2.h"
#include "Math/Mathf.h"

#include <cassert>
#include <format>


const ou::Vector2 ou::Vector2::Zero = {0.0f, 0.0f};
const ou::Vector2 ou::Vector2::One = {1.0f, 1.0f};
const ou::Vector2 ou::Vector2::Up = {0.0f, 1.0f};
const ou::Vector2 ou::Vector2::Down = {0.0f, -1.0f};
const ou::Vector2 ou::Vector2::Left = {-1.0f, 0.0f};
const ou::Vector2 ou::Vector2::Right = {1.0f, 0.0f};


ou::Vector2::Vector2(float x, float y)
	: x(x),
	  y(y) {}


float ou::Vector2::Length() const {
	return Mathf::Sqrt(Length2());
}


float ou::Vector2::Length2() const {
	return x * x + y * y;
}


void ou::Vector2::Normalize() {
	float length = Length();
	if (length == 0.0f) { return; }
	*this /= length;
}


std::string ou::Vector2::ToString() const {
	return std::format("({}, {})", x, y);
}


ou::Vector2 ou::Vector2::Normalize(const Vector2 &v) {
	auto result = v;
	result.Normalize();
	return result;
}


float ou::Vector2::Distance(const Vector2 &v1, const Vector2 &v2) {
	return (v2 - v1).Length();
}


float ou::Vector2::Dot(const Vector2 &v1, const Vector2 &v2) {
	return v1.x * v2.x + v1.y * v2.y;
}


float ou::Vector2::Cross(const Vector2 &v1, const Vector2 &v2) {
	return v1.x * v2.y - v1.y * v2.x;
}


ou::Vector2 ou::Vector2::Min(const Vector2 &v1, const Vector2 &v2) {
	return {Mathf::Min(v1.x, v2.x), Mathf::Min(v1.y, v2.y)};
}


ou::Vector2 ou::Vector2::Max(const Vector2 &v1, const Vector2 &v2) {
	return {std::max(v1.x, v2.x), std::max(v1.y, v2.y)};
}


namespace ou {

Vector2 operator+(const Vector2 &v1, const Vector2 &v2) {
	return {v1.x + v2.x, v1.y + v2.y};
}


Vector2 operator-(const Vector2 &v1, const Vector2 &v2) {
	return {v1.x - v2.x, v1.y - v2.y};
}


Vector2 operator*(const Vector2 &v1, float v2) {
	return {v1.x * v2, v1.y * v2};
}


Vector2 operator*(float v1, const Vector2 &v2) {
	return v2 * v1;
}


Vector2 operator/(const Vector2 &v1, float v2) {
	return {v1.x / v2, v1.y / v2};
}


Vector2 operator/(float v1, const Vector2 &v2) {
	return {v1 / v2.x, v1 / v2.y};
}

} // namespace ou


ou::Vector2 &ou::Vector2::operator+=(const Vector2 &v) {
	return *this = *this + v;
}


ou::Vector2 &ou::Vector2::operator-=(const Vector2 &v) {
	return *this = *this - v;
}


ou::Vector2 &ou::Vector2::operator*=(float v) {
	return *this = *this * v;
}


ou::Vector2 &ou::Vector2::operator/=(float v) {
	return *this = *this / v;
}


float &ou::Vector2::operator[](int index) {
	assert(0 <= index && index < 2);
	return (&x)[index];
}


float ou::Vector2::operator[](int index) const {
	assert(0 <= index && index < 2);
	return (&x)[index];
}


ou::Vector2 ou::Vector2::operator-() const {
	return {-x, -y};
}
