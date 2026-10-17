#include "Vector4.h"
#include "Core/Assert.h"
#include "Math/Mathf.h"

#include <format>

using namespace ou;


const Vector4 ou::Vector4::Zero = {0.0f, 0.0f, 0.0f, 0.0f};
const Vector4 ou::Vector4::One = {1.0f, 1.0f, 1.0f, 1.0f};
const Vector4 ou::Vector4::PositiveInifinity = {Mathf::Infinity,
												Mathf::Infinity,
												Mathf::Infinity,
												Mathf::Infinity};
const Vector4 ou::Vector4::NegativeInifinity = {Mathf::NegativeInfinity,
												Mathf::NegativeInfinity,
												Mathf::NegativeInfinity,
												Mathf::NegativeInfinity};


ou::Vector4::Vector4()
	: x(0.0f),
	  y(0.0f),
	  z(0.0f),
	  w(0.0f) {}


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


ou::Vector4::Vector4(const Vector2 &v)
	: x(v.x),
	  y(v.y),
	  z(0.0f),
	  w(0.0f) {}


ou::Vector4::Vector4(const Vector3 &v)
	: x(v.x),
	  y(v.y),
	  z(v.z),
	  w(0.0f) {}


Vector2 ou::Vector4::XY() const {
	return {x, y};
}


Vector2 ou::Vector4::YZ() const {
	return {y, z};
}


Vector2 ou::Vector4::ZX() const {
	return {z, x};
}


Vector3 ou::Vector4::XYZ() const {
	return {x, y, z};
}


Vector3 ou::Vector4::ZYX() const {
	return {z, y, x};
}


bool ou::Vector4::Equals(const Vector4 &other) const {
	return x == other.x &&
		   y == other.y &&
		   z == other.z &&
		   w == other.w;
}


float ou::Vector4::Length() const {
	return Mathf::Sqrt(LengthSquared());
}


float ou::Vector4::LengthSquared() const {
	return x * x +
		   y * y +
		   z * z +
		   w * w;
}


Vector4 ou::Vector4::Normalize() const {
	float len = Length();
	if (len == 0.0f) { return Vector4::Zero; }

	return {
		x / len,
		y / len,
		z / len,
		w / len};
}


std::string ou::Vector4::ToString() const {
	return std::format("({:.2f}, {:.2f}, {:.2f}, {:.2f})", x, y, z, w);
}


float ou::Vector4::Distance(const Vector4 &v1, const Vector4 &v2) {
	return (v2 - v1).Length();
}


Vector4 ou::Vector4::Normalize(Vector4 &v) {
	return v = v.Normalize();
}


float ou::Vector4::Dot(const Vector4 &v1, const Vector4 &v2) {
	return v1.x * v2.x +
		   v1.y * v2.y +
		   v1.z * v2.z +
		   v1.w * v2.w;
}


Vector4 ou::Vector4::Min(const Vector4 &v1, const Vector4 &v2) {
	return {Mathf::Min(v1.x, v2.x),
			Mathf::Min(v1.y, v2.y),
			Mathf::Min(v1.z, v2.z),
			Mathf::Min(v1.w, v2.w)};
}


Vector4 ou::Vector4::Max(const Vector4 &v1, const Vector4 &v2) {
	return {Mathf::Max(v1.x, v2.x),
			Mathf::Max(v1.y, v2.y),
			Mathf::Max(v1.z, v2.z),
			Mathf::Max(v1.w, v2.w)};
}


Vector4 ou::Vector4::Lerp(const Vector4 &a, const Vector4 &b, float t) {
	t = Mathf::Clamp01(t);
	return LerpUnclamped(a, b, t);
}


Vector4 ou::Vector4::LerpUnclamped(const Vector4 &a, const Vector4 &b, float t) {
	return a + (b - a) * t;
}


namespace ou {

Vector4 operator+(const Vector4 &v1, const Vector4 &v2) {
	return {v1.x + v2.x,
			v1.y + v2.y,
			v1.z + v2.z,
			v1.w + v2.w};
}


Vector4 operator-(const Vector4 &v1, const Vector4 &v2) {
	return {v1.x - v2.x,
			v1.y - v2.y,
			v1.z - v2.z,
			v1.w - v2.w};
}


Vector4 operator*(const Vector4 &v1, float v2) {
	return {v1.x * v2,
			v1.y * v2,
			v1.z * v2,
			v1.w * v2};
}


Vector4 operator*(float v1, const Vector4 &v2) {
	return v2 * v1;
}


Vector4 operator/(const Vector4 &v1, float v2) {
	return {v1.x / v2,
			v1.y / v2,
			v1.z / v2,
			v1.w / v2};
}


Vector4 operator/(float v1, const Vector4 &v2) {
	return {v1 / v2.x,
			v1 / v2.y,
			v1 / v2.z,
			v1 / v2.w};
}


bool operator==(const Vector4 &a, const Vector4 &b) {
	return Mathf::Approximately(a.x, b.x) &&
		   Mathf::Approximately(a.y, b.y) &&
		   Mathf::Approximately(a.z, b.z) &&
		   Mathf::Approximately(a.w, b.w);
}

} // namespace ou


Vector4 &ou::Vector4::operator+=(const Vector4 &v) {
	return *this = *this + v;
}


Vector4 &ou::Vector4::operator-=(const Vector4 &v) {
	return *this = *this - v;
}


Vector4 &ou::Vector4::operator*=(float v) {
	return *this = *this * v;
}


Vector4 &ou::Vector4::operator/=(float v) {
	return *this = *this / v;
}


float &ou::Vector4::operator[](int index) {
	switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		case 3:
			return w;
		default:
			ASSERT(false, "範囲外のインデックスです: {}", index);
			return x;
	}
}


float ou::Vector4::operator[](int index) const {
	switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		case 3:
			return w;
		default:
			ASSERT(false, "範囲外のインデックスです: {}", index);
			return x;
	}
}


ou::Vector4 ou::Vector4::operator-() const {
	return {-x, -y, -z, -w};
}


ou::Vector4::operator Vector2() const {
	return {x, y};
}


ou::Vector4::operator Vector3() const {
	return {x, y, z};
}
