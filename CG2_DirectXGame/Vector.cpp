#include "Vector.h"
#include <cmath>
#include <cassert>


// Vector2 ========================================================================================

Vector2 operator+(const Vector2 &v1, const Vector2 &v2) {
	return Vector2(v1.x + v2.x, v1.y + v2.y);
}


Vector2 operator-(const Vector2 &v1, const Vector2 &v2) {
	return Vector2(v1.x - v2.x, v1.y - v2.y);
}


Vector2 operator*(const Vector2 &v, float scalar) {
	return Vector2(v.x * scalar, v.y * scalar);
}


Vector2 operator*(float scalar, const Vector2 &v) {
	return v * scalar;
}


Vector2 operator/(const Vector2 &v, float scalar) {
	assert(scalar != 0.0f);

	if (scalar == 0.0f) {
		return Vector2();
	}

	return Vector2(v.x / scalar, v.y / scalar);
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
	return Vector2(x, y);
}


Vector2 Vector2::operator-() const {
	return Vector2(-x, -y);
}

// ======================================================================================== Vector2



// Vector3 ========================================================================================

Vector3 operator+(const Vector3 &v1, const Vector3 &v2) {
	return Vector3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}


Vector3 operator-(const Vector3 &v1, const Vector3 &v2) {
	return Vector3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}


Vector3 operator*(const Vector3 &v, float scalar) {
	return Vector3(v.x * scalar, v.y * scalar, v.z * scalar);
}


Vector3 operator*(float scalar, const Vector3 &v) {
	return v * scalar;
}


Vector3 operator/(const Vector3 &v, float scalar) {
	assert(scalar != 0.0f);

	if (scalar == 0.0f) {
		return Vector3();
	}

	return Vector3(v.x / scalar, v.y / scalar, v.z / scalar);
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
	return Vector3(-x, -y, -z);
}

// ======================================================================================== Vector3



namespace MathUtility {

	// Vector2 ====================================================================================

	float Dot(const Vector2 &v1, const Vector2 &v2) {
		return v1.x * v2.x + v1.y * v2.y;
	}


	float Cross(const Vector2 &v1, const Vector2 &v2) {
		return v1.x * v2.y - v1.y * v2.x;
	}


	float Length(const Vector2 &v) {
		return std::sqrt(v.x * v.x + v.y * v.y);
	}


	Vector2 Normalize(const Vector2 &v) {
		float len = Length(v);

		if (len == 0.0f) {
			return Vector2();
		}

		return Vector2(v.x / len, v.y / len);
	}

	// ==================================================================================== Vector2



	// Vector3 ====================================================================================

	float Dot(const Vector3 &v1, const Vector3 &v2) {
		return v1.x * v2.x
			+ v1.y * v2.y
			+ v1.z * v2.z;
	}


	Vector3 Cross(const Vector3 &v1, const Vector3 &v2) {
		return Vector3(
			v1.y * v2.z - v1.z * v2.y,
			v1.z * v2.x - v1.x * v2.z,
			v1.x * v2.y - v1.y * v2.x);
	}


	float Length(const Vector3 &v) {
		return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	}


	float LengthSq(const Vector3 &v) {
		return Dot(v, v);
	}


	Vector3 Normalize(const Vector3 &v) {
		float len = Length(v);

		if (len == 0.0f) {
			return Vector3();
		}

		return Vector3(v.x / len, v.y / len, v.z / len);
	}


	Vector3 Lerp(const Vector3 &v1, const Vector3 &v2, float t) {
		return Vector3(
			v1.x + (v2.x - v1.x) * t,
			v1.y + (v2.y - v1.y) * t,
			v1.z + (v2.z - v1.z) * t);
	}


	Vector3 Bezier(const Vector3 &controlPoint0, const Vector3 &controlPoint1, const Vector3 &controlPoint2, float t) {
		Vector3 cp01 = Lerp(controlPoint0, controlPoint1, t);
		Vector3 cp12 = Lerp(controlPoint1, controlPoint2, t);

		return Lerp(cp01, cp12, t);
	}

	// ==================================================================================== Vector3

} // MathUtility