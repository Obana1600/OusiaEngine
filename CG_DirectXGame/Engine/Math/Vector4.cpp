#include "Vector4.h"
#include "Math/Mathf.h"

#include <cassert>
#include <format>


const ou::Vector4 ou::Vector4::Zero = {0.0f, 0.0f, 0.0f, 0.0f};
const ou::Vector4 ou::Vector4::One = {1.0f, 1.0f, 1.0f, 1.0f};


ou::Vector4::Vector4(float x, float y, float z, float w)
	: x(x),
	  y(y),
	  z(z),
	  w(w) {}


ou::Vector4::Vector4(const Vector2 &vec, float z, float w)
	: x(vec.x),
	  y(vec.y),
	  z(z),
	  w(w) {}


ou::Vector4::Vector4(const Vector3 &vec, float w)
	: x(vec.x),
	  y(vec.y),
	  z(vec.z),
	  w(w) {}


ou::Vector2 ou::Vector4::XY() const {
	return {x, y};
}


ou::Vector2 ou::Vector4::YZ() const {
	return {y, z};
}


ou::Vector2 ou::Vector4::ZX() const {
	return {z, x};
}


ou::Vector3 ou::Vector4::XYZ() const {
	return {x, y, z};
}


ou::Vector3 ou::Vector4::ZYX() const {
	return {z, y, x};
}


float ou::Vector4::Length() const {
	return Mathf::Sqrt(Length2());
}


float ou::Vector4::Length2() const {
	return x * x + y * y + z * z + w * w;
}


void ou::Vector4::Normalize() {
	float length = Length();
	if (length == 0.0f) { return; }
	*this /= length;
}


std::string ou::Vector4::ToString() const {
	return std::format("({}, {}, {}, {})", x, y, z, w);
}


float ou::Vector4::Distance(const Vector4 &v1, const Vector4 &v2) {
	return (v2 - v1).Length();
}


ou::Vector4 ou::Vector4::Normalize(const Vector4 &v) {
	auto result = v;
	result.Normalize();
	return result;
}


float ou::Vector4::Dot(const Vector4 &v1, const Vector4 &v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}


ou::Vector4 ou::Vector4::Min(const Vector4 &v1, const Vector4 &v2) {
	return {Mathf::Min(v1.x, v2.x), Mathf::Min(v1.y, v2.y), Mathf::Min(v1.z, v2.z), Mathf::Min(v1.w, v2.w)};
}


ou::Vector4 ou::Vector4::Max(const Vector4 &v1, const Vector4 &v2) {
	return {Mathf::Max(v1.x, v2.x), Mathf::Max(v1.y, v2.y), Mathf::Max(v1.z, v2.z), Mathf::Max(v1.w, v2.w)};
}


namespace ou {

Vector4 operator+(const Vector4 &v1, const Vector4 &v2) {
	return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w};
}


Vector4 operator-(const Vector4 &v1, const Vector4 &v2) {
	return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w};
}


Vector4 operator*(const Vector4 &v1, float v2) {
	return {v1.x * v2, v1.y * v2, v1.z * v2, v1.w * v2};
}


Vector4 operator*(float v1, const Vector4 &v2) {
	return v2 * v1;
}


Vector4 operator/(const Vector4 &v1, float v2) {
	return {v1.x / v2, v1.y / v2, v1.z / v2, v1.w / v2};
}


Vector4 operator/(float v1, const Vector4 &v2) {
	return {v1 / v2.x, v1 / v2.y, v1 / v2.z, v1 / v2.w};
}

} // namespace ou


ou::Vector4 &ou::Vector4::operator+=(const Vector4 &v) {
	return *this = *this + v;
}


ou::Vector4 &ou::Vector4::operator-=(const Vector4 &v) {
	return *this = *this - v;
}


ou::Vector4 &ou::Vector4::operator*=(float v) {
	return *this = *this * v;
}


ou::Vector4 &ou::Vector4::operator/=(float v) {
	return *this = *this / v;
}


float &ou::Vector4::operator[](int index) {
	assert(0 <= index && index < 4);
	return (&x)[index];
}


float ou::Vector4::operator[](int index) const {
	assert(0 <= index && index < 4);
	return (&x)[index];
}


ou::Vector4 ou::Vector4::operator-() const {
	return {-x, -y, -z, -w};
}
