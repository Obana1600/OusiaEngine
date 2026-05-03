#include "Vector.h"
#include <cmath>


namespace MathUtility {

	Vector3 Add(const Vector3 &v1, const Vector3 &v2) {
		return v1 + v2;
	}

	Vector3 Subtract(const Vector3 &v1, const Vector3 &v2) {
		return v1 - v2;
	}

	Vector3 Multiply(float scalar, const Vector3 &v) {
		return scalar * v;
	}


	Vector3 Multiply(const Vector3 &v, float scalar) {
		return v * scalar;
	}


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


	Vector3 Normalize(const Vector3 &v) {
		float len = Length(v);

		if (len == 0.0f) {
			return Vector3();
		}

		return Vector3(v.x / len, v.y / len, v.z / len);
	}

}