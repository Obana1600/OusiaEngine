#include "Vector3.h"
#include "Math/Mathf.h"

#include <cassert>
#include <format>


const ou::Vector3 ou::Vector3::Zero = {0.0f, 0.0f, 0.0f};
const ou::Vector3 ou::Vector3::One = {1.0f, 1.0f, 1.0f};
const ou::Vector3 ou::Vector3::Up = {0.0f, 1.0f, 0.0f};
const ou::Vector3 ou::Vector3::Down = {0.0f, -1.0f, 0.0f};
const ou::Vector3 ou::Vector3::Left = {-1.0f, 0.0f, 0.0f};
const ou::Vector3 ou::Vector3::Right = {1.0f, 0.0f, 0.0f};
const ou::Vector3 ou::Vector3::Forward = {0.0f, 0.0f, 1.0f};
const ou::Vector3 ou::Vector3::Back = {0.0f, 0.0f, -1.0f};


ou::Vector3::Vector3(float x, float y, float z)
	: x(x),
	  y(y),
	  z(z) {}


ou::Vector3::Vector3(const Vector2 &vec, float z)
	: x(vec.x),
	  y(vec.y),
	  z(z) {}


ou::Vector2 ou::Vector3::XY() const {
	return {x, y};
}


ou::Vector2 ou::Vector3::YZ() const {
	return {y, z};
}


ou::Vector2 ou::Vector3::ZX() const {
	return {z, x};
}


float ou::Vector3::Length() const {
	return Mathf::Sqrt(Length2());
}


float ou::Vector3::Length2() const {
	return x * x + y * y + z * z;
}


void ou::Vector3::Normalize() {
	float length = Length();
	if (length == 0.0f) { return; }
	*this /= length;
}


std::string ou::Vector3::ToString() const {
	return std::format("({}, {}, {})", x, y, z);
}


float ou::Vector3::Distance(const Vector3 &v1, const Vector3 &v2) {
	return (v2 - v1).Length();
}


ou::Vector3 ou::Vector3::Normalize(const Vector3 &v) {
	auto result = v;
	result.Normalize();
	return result;
}


float ou::Vector3::Dot(const Vector3 &v1, const Vector3 &v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}


ou::Vector3 ou::Vector3::Cross(const Vector3 &v1, const Vector3 &v2) {
	return {
		v1.y * v2.z - v1.z * v2.y,
		v1.z * v2.x - v1.x * v2.z,
		v1.x * v2.y - v1.y * v2.x};
}


ou::Vector3 ou::Vector3::Min(const Vector3 &v1, const Vector3 &v2) {
	return {Mathf::Min(v1.x, v2.x), Mathf::Min(v1.y, v2.y), Mathf::Min(v1.z, v2.z)};
}


ou::Vector3 ou::Vector3::Max(const Vector3 &v1, const Vector3 &v2) {
	return {Mathf::Max(v1.x, v2.x), Mathf::Max(v1.y, v2.y), Mathf::Max(v1.z, v2.z)};
}


namespace ou {

Vector3 operator+(const Vector3 &v1, const Vector3 &v2) {
	return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}


Vector3 operator-(const Vector3 &v1, const Vector3 &v2) {
	return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}


Vector3 operator*(const Vector3 &v1, float v2) {
	return {v1.x * v2, v1.y * v2, v1.z * v2};
}


Vector3 operator*(float v1, const Vector3 &v2) {
	return v2 * v1;
}


Vector3 operator/(const Vector3 &v1, float v2) {
	return {v1.x / v2, v1.y / v2, v1.z / v2};
}


Vector3 operator/(float v1, const Vector3 &v2) {
	return {v1 / v2.x, v1 / v2.y, v1 / v2.z};
}

} // namespace ou


ou::Vector3 &ou::Vector3::operator+=(const Vector3 &v) {
	return *this = *this + v;
}


ou::Vector3 &ou::Vector3::operator-=(const Vector3 &v) {
	return *this = *this - v;
}


ou::Vector3 &ou::Vector3::operator*=(float v) {
	return *this = *this * v;
}


ou::Vector3 &ou::Vector3::operator/=(float v) {
	return *this = *this / v;
}


float &ou::Vector3::operator[](int index) {
	assert(0 <= index && index < 3);
	return (&x)[index];
}


float ou::Vector3::operator[](int index) const {
	assert(0 <= index && index < 3);
	return (&x)[index];
}


ou::Vector3 ou::Vector3::operator-() const {
	return {-x, -y, -z};
}
