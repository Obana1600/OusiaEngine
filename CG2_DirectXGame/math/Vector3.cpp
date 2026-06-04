#include "Vector3.h"
#include <cassert>


Vector3 operator+(const Vector3 &v1, const Vector3 &v2) {
	return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}


Vector3 operator-(const Vector3 &v1, const Vector3 &v2) {
	return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}


Vector3 operator*(const Vector3 &v, float scalar) {
	return { v.x * scalar, v.y * scalar, v.z * scalar };
}


Vector3 operator*(float scalar, const Vector3 &v) {
	return v * scalar;
}


Vector3 operator/(const Vector3 &v, float scalar) {
	assert(scalar != 0.0f);

	if (scalar == 0.0f) {
		return { 0.0f, 0.0f, 0.0f };
	}

	return { v.x / scalar, v.y / scalar, v.z / scalar };
}


Vector3 &Vector3::operator+=(const Vector3 &other) {
	*this = *this + other;
	return *this;
}


Vector3 &Vector3::operator-=(const Vector3 &other) {
	*this = *this - other;
	return *this;
}


Vector3 &Vector3::operator*=(float scalar) {
	*this = *this * scalar;
	return *this;
}


Vector3 &Vector3::operator/=(float scalar) {
	*this = *this / scalar;
	return *this;
}


Vector3 Vector3::operator+() const {
	return *this;
}


Vector3 Vector3::operator-() const {
	return { -x, -y, -z };
}