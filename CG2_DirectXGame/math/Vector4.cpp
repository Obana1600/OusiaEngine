#include "Vector4.h"
#include <cassert>


Vector4 operator+(const Vector4 &v1, const Vector4 &v2) {
	return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w};
}


Vector4 operator-(const Vector4 &v1, const Vector4 &v2) {
	return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w};
}


Vector4 operator*(const Vector4 &v, float scalar) {
	return {v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar};
}


Vector4 operator*(float scalar, const Vector4 &v) {
	return v * scalar;
}


Vector4 operator/(const Vector4 &v, float scalar) {
	assert(scalar != 0.0f);

	if (scalar == 0.0f) {
		return {0.0f, 0.0f, 0.0f, 0.0f};
	}

	return {v.x / scalar, v.y / scalar, v.z / scalar, v.w / scalar};
}


Vector4 &Vector4::operator+=(const Vector4 &other) {
	*this = *this + other;
	return *this;
}


Vector4 &Vector4::operator-=(const Vector4 &other) {
	*this = *this - other;
	return *this;
}


Vector4 &Vector4::operator*=(float scalar) {
	*this = *this * scalar;
	return *this;
}


Vector4 &Vector4::operator/=(float scalar) {
	*this = *this / scalar;
	return *this;
}


Vector4 Vector4::operator+() const {
	return *this;
}


Vector4 Vector4::operator-() const {
	return {-x, -y, -z, -w};
}


float Vector4::operator[](int index) {
	assert(index < 4);
	return (&x)[index];
}


const float &Vector4::operator[](int index) const {
	assert(index < 4);
	return (&x)[index];
}