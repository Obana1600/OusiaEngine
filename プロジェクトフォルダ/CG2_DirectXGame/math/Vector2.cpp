#include "Vector2.h"
#include <cassert>


Vector2 operator+(const Vector2 &v1, const Vector2 &v2) {
	return {v1.x + v2.x, v1.y + v2.y};
}


Vector2 operator-(const Vector2 &v1, const Vector2 &v2) {
	return {v1.x - v2.x, v1.y - v2.y};
}


Vector2 operator*(const Vector2 &v, float scalar) {
	return {v.x * scalar, v.y * scalar};
}


Vector2 operator*(float scalar, const Vector2 &v) {
	return v * scalar;
}


Vector2 operator/(const Vector2 &v, float scalar) {
	assert(scalar != 0.0f);

	if (scalar == 0.0f) {
		return {0.0f, 0.0f};
	}

	return {v.x / scalar, v.y / scalar};
}


Vector2 &Vector2::operator+=(const Vector2 &other) {
	*this = *this + other;
	return *this;
}


Vector2 &Vector2::operator-=(const Vector2 &other) {
	*this = *this - other;
	return *this;
}


Vector2 &Vector2::operator*=(float scalar) {
	*this = *this * scalar;
	return *this;
}


Vector2 &Vector2::operator/=(float scalar) {
	*this = *this / scalar;
	return *this;
}


Vector2 Vector2::operator+() const {
	return {x, y};
}


Vector2 Vector2::operator-() const {
	return {-x, -y};
}


float Vector2::operator[](int index) {
	assert(index < 2);
	return (&x)[index];
}


const float &Vector2::operator[](int index) const {
	assert(index < 2);
	return (&x)[index];
}