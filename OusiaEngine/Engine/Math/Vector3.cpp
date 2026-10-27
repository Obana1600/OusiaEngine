#include "Vector3.h"
#include "Diagnostics/Assert.h"
#include "Math/Mathf.h"

#include <format>

using namespace ou;


const Vector3 ou::Vector3::Zero = {0.0f, 0.0f, 0.0f};
const Vector3 ou::Vector3::One = {1.0f, 1.0f, 1.0f};
const Vector3 ou::Vector3::Up = {0.0f, 1.0f, 0.0f};
const Vector3 ou::Vector3::Down = {0.0f, -1.0f, 0.0f};
const Vector3 ou::Vector3::Left = {-1.0f, 0.0f, 0.0f};
const Vector3 ou::Vector3::Right = {1.0f, 0.0f, 0.0f};
const Vector3 ou::Vector3::Forward = {0.0f, 0.0f, 1.0f};
const Vector3 ou::Vector3::Back = {0.0f, 0.0f, -1.0f};
const Vector3 ou::Vector3::PositiveInfinity = {Mathf::Infinity, Mathf::Infinity, Mathf::Infinity};
const Vector3 ou::Vector3::NegativeInfinity = {Mathf::NegativeInfinity, Mathf::NegativeInfinity, Mathf::NegativeInfinity};


ou::Vector3::Vector3()
	: x(0.0f),
	  y(0.0f),
	  z(0.0f) {}


ou::Vector3::Vector3(float x, float y, float z)
	: x(x),
	  y(y),
	  z(z) {}


ou::Vector3::Vector3(const Vector2 &vec, float z)
	: x(vec.x),
	  y(vec.y),
	  z(z) {}


Vector2 ou::Vector3::XY() const {
	return {x, y};
}


Vector2 ou::Vector3::YZ() const {
	return {y, z};
}


Vector2 ou::Vector3::ZX() const {
	return {z, x};
}


bool ou::Vector3::Equals(const Vector3 &other) const {
	return x == other.x &&
		   y == other.y &&
		   z == other.z;
}


float ou::Vector3::Length() const {
	return Mathf::Sqrt(LengthSquared());
}


float ou::Vector3::LengthSquared() const {
	return x * x +
		   y * y +
		   z * z;
}


Vector3 ou::Vector3::Normalize() const {
	float len = Length();
	if (len == 0.0f) { return Vector3::Zero; }

	return {x / len,
			y / len,
			z / len};
}


std::string ou::Vector3::ToString() const {
	return std::format("({:.2f}, {:.2f}, {:.2f})", x, y, z);
}


std::string ou::Vector3::ToString(std::string_view format) const {
	const auto fmt = std::format("({{:{0}}}, {{:{0}}}, {{:{0}}})", format);
	return std::vformat(fmt, std::make_format_args(x, y, z));
}


float ou::Vector3::Distance(const Vector3 &v1, const Vector3 &v2) {
	return (v2 - v1).Length();
}


Vector3 ou::Vector3::Normalize(Vector3 &v) {
	return v = v.Normalize();
}


float ou::Vector3::Dot(const Vector3 &v1, const Vector3 &v2) {
	return v1.x * v2.x +
		   v1.y * v2.y +
		   v1.z * v2.z;
}


Vector3 ou::Vector3::Cross(const Vector3 &v1, const Vector3 &v2) {
	return {(v1.y * v2.z) - (v1.z * v2.y),
			(v1.z * v2.x) - (v1.x * v2.z),
			(v1.x * v2.y) - (v1.y * v2.x)};
}


Vector3 ou::Vector3::Min(const Vector3 &v1, const Vector3 &v2) {
	return {Mathf::Min(v1.x, v2.x),
			Mathf::Min(v1.y, v2.y),
			Mathf::Min(v1.z, v2.z)};
}


Vector3 ou::Vector3::Max(const Vector3 &v1, const Vector3 &v2) {
	return {Mathf::Max(v1.x, v2.x),
			Mathf::Max(v1.y, v2.y),
			Mathf::Max(v1.z, v2.z)};
}


Vector3 ou::Vector3::Lerp(const Vector3 &a, const Vector3 &b, float t) {
	t = Mathf::Clamp01(t);
	return LerpUnclamped(a, b, t);
}


Vector3 ou::Vector3::LerpUnclamped(const Vector3 &a, const Vector3 &b, float t) {
	return a + (b - a) * t;
}


Vector3 ou::Vector3::Scale(const Vector3 &a, const Vector3 &b) {
	return {a.x * b.x, a.y * b.y, a.z * b.z};
}


namespace ou {

Vector3 operator+(const Vector3 &v1, const Vector3 &v2) {
	return {v1.x + v2.x,
			v1.y + v2.y,
			v1.z + v2.z};
}


Vector3 operator-(const Vector3 &v1, const Vector3 &v2) {
	return {v1.x - v2.x,
			v1.y - v2.y,
			v1.z - v2.z};
}


Vector3 operator*(const Vector3 &v1, float v2) {
	return {v1.x * v2,
			v1.y * v2,
			v1.z * v2};
}


Vector3 operator*(float v1, const Vector3 &v2) {
	return v2 * v1;
}


Vector3 operator/(const Vector3 &v1, float v2) {
	return {v1.x / v2,
			v1.y / v2,
			v1.z / v2};
}


Vector3 operator/(float v1, const Vector3 &v2) {
	return {v1 / v2.x,
			v1 / v2.y,
			v1 / v2.z};
}


bool operator==(const Vector3 &a, const Vector3 &b) {
	return Mathf::Approximately(a.x, b.x) &&
		   Mathf::Approximately(a.y, b.y) &&
		   Mathf::Approximately(a.z, b.z);
}

} // namespace ou


Vector3 &ou::Vector3::operator+=(const Vector3 &v) {
	return *this = *this + v;
}


Vector3 &ou::Vector3::operator-=(const Vector3 &v) {
	return *this = *this - v;
}


Vector3 &ou::Vector3::operator*=(float v) {
	return *this = *this * v;
}


Vector3 &ou::Vector3::operator/=(float v) {
	return *this = *this / v;
}


float &ou::Vector3::operator[](int index) {
	switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		default:
			ASSERT(false, "範囲外のインデックスです: {}", index);
			return x;
	}
}


float ou::Vector3::operator[](int index) const {
	switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		default:
			ASSERT(false, "範囲外のインデックスです: {}", index);
			return x;
	}
}


Vector3 ou::Vector3::operator-() const {
	return {-x, -y, -z};
}
