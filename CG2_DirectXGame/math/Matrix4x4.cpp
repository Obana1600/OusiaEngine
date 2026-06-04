#include "Matrix4x4.h"


Matrix4x4 operator+(const Matrix4x4 &m1, const Matrix4x4 &m2) {
	Matrix4x4 result{};

	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			result.m[row][col] = m1.m[row][col] + m2.m[row][col];
		}
	}

	return result;
}


Matrix4x4 operator-(const Matrix4x4 &m1, const Matrix4x4 &m2) {
	Matrix4x4 result{};

	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			result.m[row][col] = m1.m[row][col] - m2.m[row][col];
		}
	}

	return result;
}


Matrix4x4 operator*(const Matrix4x4 &m1, const Matrix4x4 &m2) {
	Matrix4x4 result{};

	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			for (int k = 0; k < 4; k++) {
				result.m[row][col] += m1.m[row][k] * m2.m[k][col];
			}
		}
	}

	return result;
}


Matrix4x4 &Matrix4x4::operator+=(const Matrix4x4 &other) {
	*this = *this + other;
	return *this;
}


Matrix4x4 &Matrix4x4::operator-=(const Matrix4x4 &other) {
	*this = *this - other;
	return *this;
}


Matrix4x4 &Matrix4x4::operator*=(const Matrix4x4 &other) {
	*this = *this * other;
	return *this;
}