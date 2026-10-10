#include "Matrix4x4.h"
#include "Math/Mathf.h"

#include <cassert>
#include <cmath>
#include <format>


namespace {

// 2x2 小行列式
struct MinorTerms {
	float s0, s1, s2, s3, s4, s5;
	float c0, c1, c2, c3, c4, c5;

	explicit MinorTerms(const float (&a)[4][4]) {
		s0 = a[0][0] * a[1][1] - a[1][0] * a[0][1];
		s1 = a[0][0] * a[1][2] - a[1][0] * a[0][2];
		s2 = a[0][0] * a[1][3] - a[1][0] * a[0][3];
		s3 = a[0][1] * a[1][2] - a[1][1] * a[0][2];
		s4 = a[0][1] * a[1][3] - a[1][1] * a[0][3];
		s5 = a[0][2] * a[1][3] - a[1][2] * a[0][3];

		c5 = a[2][2] * a[3][3] - a[3][2] * a[2][3];
		c4 = a[2][1] * a[3][3] - a[3][1] * a[2][3];
		c3 = a[2][1] * a[3][2] - a[3][1] * a[2][2];
		c2 = a[2][0] * a[3][3] - a[3][0] * a[2][3];
		c1 = a[2][0] * a[3][2] - a[3][0] * a[2][2];
		c0 = a[2][0] * a[3][1] - a[3][0] * a[2][1];
	}

	float Determinant() const {
		return s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
	}
};

} // namespace



const ou::Matrix4x4 ou::Matrix4x4::Identity = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f};


const ou::Matrix4x4 ou::Matrix4x4::Zero = {
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f};


ou::Matrix4x4::Matrix4x4(
	float m11, float m12, float m13, float m14,
	float m21, float m22, float m23, float m24,
	float m31, float m32, float m33, float m34,
	float m41, float m42, float m43, float m44)
	: m11(m11), m12(m12), m13(m13), m14(m14),
	  m21(m21), m22(m22), m23(m23), m24(m24),
	  m31(m31), m32(m32), m33(m33), m34(m34),
	  m41(m41), m42(m42), m43(m43), m44(m44) {}


ou::Matrix4x4::Matrix4x4(const Vector4 &row1, const Vector4 &row2, const Vector4 &row3, const Vector4 &row4) {
	SetRow(0, row1);
	SetRow(1, row2);
	SetRow(2, row3);
	SetRow(3, row4);
}


ou::Vector4 ou::Matrix4x4::GetColumn(int index) const {
	assert(0 <= index && index < 4);
	return {m[0][index], m[1][index], m[2][index], m[3][index]};
}


void ou::Matrix4x4::SetColumn(int index, const Vector4 &v) {
	assert(0 <= index && index < 4);

	for (int row = 0; row < 4; row++) {
		m[row][index] = v[row];
	}
}


ou::Vector4 ou::Matrix4x4::GetRow(int index) const {
	assert(0 <= index && index < 4);
	return {m[index][0], m[index][1], m[index][2], m[index][3]};
}


void ou::Matrix4x4::SetRow(int index, const Vector4 &v) {
	assert(0 <= index && index < 4);

	for (int column = 0; column < 4; column++) {
		m[index][column] = v[column];
	}
}


float ou::Matrix4x4::Determinant() const {
	return MinorTerms(m).Determinant();
}


ou::Matrix4x4 ou::Matrix4x4::Inverse() const {
	const MinorTerms t(m);
	const auto det = t.Determinant();
	assert(Mathf::Abs(det) > Mathf::Epsilon && "逆行列が存在しない");

	Matrix4x4 result = {
		m[1][1] * t.c5 - m[1][2] * t.c4 + m[1][3] * t.c3,
		-m[0][1] * t.c5 + m[0][2] * t.c4 - m[0][3] * t.c3,
		m[3][1] * t.s5 - m[3][2] * t.s4 + m[3][3] * t.s3,
		-m[2][1] * t.s5 + m[2][2] * t.s4 - m[2][3] * t.s3,

		-m[1][0] * t.c5 + m[1][2] * t.c2 - m[1][3] * t.c1,
		m[0][0] * t.c5 - m[0][2] * t.c2 + m[0][3] * t.c1,
		-m[3][0] * t.s5 + m[3][2] * t.s2 - m[3][3] * t.s1,
		m[2][0] * t.s5 - m[2][2] * t.s2 + m[2][3] * t.s1,

		m[1][0] * t.c4 - m[1][1] * t.c2 + m[1][3] * t.c0,
		-m[0][0] * t.c4 + m[0][1] * t.c2 - m[0][3] * t.c0,
		m[3][0] * t.s4 - m[3][1] * t.s2 + m[3][3] * t.s0,
		-m[2][0] * t.s4 + m[2][1] * t.s2 - m[2][3] * t.s0,

		-m[1][0] * t.c3 + m[1][1] * t.c1 - m[1][2] * t.c0,
		m[0][0] * t.c3 - m[0][1] * t.c1 + m[0][2] * t.c0,
		-m[3][0] * t.s3 + m[3][1] * t.s1 - m[3][2] * t.s0,
		m[2][0] * t.s3 - m[2][1] * t.s1 + m[2][2] * t.s0};

	return result * (1.0f / det);
}


ou::Matrix4x4 ou::Matrix4x4::Transpose() const {
	return {
		m11, m21, m31, m41,
		m12, m22, m32, m42,
		m13, m23, m33, m43,
		m14, m24, m34, m44};
}


bool ou::Matrix4x4::IsIdentity() const {
	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++) {
			const auto def = Mathf::Abs(m[row][column] - m[row][column]);
			if (def > Mathf::Epsilon) {
				return false;
			}
		}
	}
	return true;
}


std::string ou::Matrix4x4::ToString() const {
	return std::format(
		"({}, {}, {}, {})\n({}, {}, {}, {})\n({}, {}, {}, {})\n({}, {}, {}, {})",
		m11, m12, m13, m14,
		m21, m22, m23, m24,
		m31, m32, m33, m34,
		m41, m42, m43, m44);
}


ou::Matrix4x4 ou::Matrix4x4::Translate(const Vector3 &v) {
	return {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		v.x, v.y, v.z, 1.0f};
}


ou::Matrix4x4 ou::Matrix4x4::Scale(const Vector3 &v) {
	return {
		v.x, 0.0f, 0.0f, 0.0f,
		0.0f, v.y, 0.0f, 0.0f,
		0.0f, 0.0f, v.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f};
}


ou::Matrix4x4 ou::Matrix4x4::Rotate(const Vector3 &v) {
	const float sx = std::sin(v.x), cx = std::cos(v.x);
	const float sy = std::sin(v.y), cy = std::cos(v.y);
	const float sz = std::sin(v.z), cz = std::cos(v.z);

	const Matrix4x4 rotateX = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, cx, sx, 0.0f,
		0.0f, -sx, cx, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f};

	const Matrix4x4 rotateY = {
		cy, 0.0f, -sy, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		sy, 0.0f, cy, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f};

	const Matrix4x4 rotateZ = {
		cz, sz, 0.0f, 0.0f,
		-sz, cz, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f};

	return rotateX * rotateY * rotateZ;
}


ou::Matrix4x4 ou::Matrix4x4::SRT(const Vector3 &s, const Vector3 &r, const Vector3 &t) {
	return Scale(s) * Rotate(r) * Translate(t);
}


ou::Matrix4x4 ou::Matrix4x4::Perspective(float fovY, float aspect, float nearZ, float farZ) {
	const float yScale = 1.0f / std::tan(fovY * 0.5f);
	const float xScale = yScale / aspect;
	const float range = farZ / (nearZ - farZ);

	return {xScale, 0.0f, 0.0f, 0.0f,
			0.0f, yScale, 0.0f, 0.0f,
			0.0f, 0.0f, range, -1.0f,
			0.0f, 0.0f, range * nearZ, 0.0f};
}


ou::Matrix4x4 ou::Matrix4x4::Ortho(float width, float height, float nearZ, float farZ) {
	const float range = 1.0f / (nearZ - farZ);

	return {2.0f / width, 0.0f, 0.0f, 0.0f,
			0.0f, 2.0f / height, 0.0f, 0.0f,
			0.0f, 0.0f, range, 0.0f,
			0.0f, 0.0f, range * nearZ, 1.0f};
}


namespace ou {

Matrix4x4 operator*(const Matrix4x4 &m1, const Matrix4x4 &m2) {
	Matrix4x4 result{};
	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++) {
			for (int k = 0; k < 4; k++) {
				result.m[row][column] += m1.m[row][k] * m2.m[k][column];
			}
		}
	}
	return result;
}


Matrix4x4 operator*(const Matrix4x4 &m1, float m2) {
	Matrix4x4 result{};
	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++) {
			result.m[row][column] = m1.m[row][column] * m2;
		}
	}
	return result;
}

} // namespace ou


ou::Matrix4x4 &ou::Matrix4x4::operator*=(const Matrix4x4 &matrix) {
	return *this = *this * matrix;
}


ou::Matrix4x4 &ou::Matrix4x4::operator*=(float scalar) {
	return *this = *this * scalar;
}


float &ou::Matrix4x4::operator()(int row, int column) {
	assert(0 <= row && row < 4 && 0 <= column && column < 4);
	return m[row][column];
}


float ou::Matrix4x4::operator()(int row, int column) const {
	assert(0 <= row && row < 4 && 0 <= column && column < 4);
	return m[row][column];
}
