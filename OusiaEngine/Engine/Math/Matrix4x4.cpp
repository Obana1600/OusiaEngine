#include "Matrix4x4.h"
#include "Diagnostics/Assert.h"
#include "Math/Mathf.h"

#include <format>

using namespace ou;


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


ou::Matrix4x4::Matrix4x4()
	: m{} {}


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


void ou::Matrix4x4::SetColumn(int index, const Vector4 &v) {
	ASSERT(0 <= index && index < 4, "範囲外のインデックスです: {}", index);

	for (int row = 0; row < 4; row++) {
		m[row][index] = v[row];
	}
}


Vector4 ou::Matrix4x4::GetColumn(int index) const {
	ASSERT(0 <= index && index < 4, "範囲外のインデックスです: {}", index);
	return {m[0][index], m[1][index], m[2][index], m[3][index]};
}


void ou::Matrix4x4::SetRow(int index, const Vector4 &v) {
	ASSERT(0 <= index && index < 4, "範囲外のインデックスです: {}", index);

	for (int column = 0; column < 4; column++) {
		m[index][column] = v[column];
	}
}


Vector4 ou::Matrix4x4::GetRow(int index) const {
	ASSERT(0 <= index && index < 4, "範囲外のインデックスです: {}", index);
	return {m[index][0], m[index][1], m[index][2], m[index][3]};
}


Vector3 ou::Matrix4x4::GetPosition() const {
	return Vector3(m41, m42, m43);
}


Quaternion ou::Matrix4x4::GetRotation() const {
	const auto scale = GetLossyScale();
	if (scale.x == 0.0f || scale.y == 0.0f || scale.z == 0.0f) { return Quaternion::Identity; }

	const auto ix = 1.0f / scale.x;
	const auto iy = 1.0f / scale.y;
	const auto iz = 1.0f / scale.z;
	const auto r11 = m11 * ix, r12 = m12 * ix, r13 = m13 * ix;
	const auto r21 = m21 * iy, r22 = m22 * iy, r23 = m23 * iy;
	const auto r31 = m31 * iz, r32 = m32 * iz, r33 = m33 * iz;

	const auto trace = r11 + r22 + r33;
	if (trace > 0.0f) {
		const auto s = Mathf::Sqrt(trace + 1.0f) * 2.0f; // 4w
		return Quaternion((r23 - r32) / s, (r31 - r13) / s, (r12 - r21) / s, 0.25f * s).Normalized();
	}
	if (r11 > r22 && r11 > r33) {
		const auto s = Mathf::Sqrt(1.0f + r11 - r22 - r33) * 2.0f; // 4x
		return Quaternion(0.25f * s, (r12 + r21) / s, (r31 + r13) / s, (r23 - r32) / s).Normalized();
	}
	if (r22 > r33) {
		const auto s = Mathf::Sqrt(1.0f + r22 - r11 - r33) * 2.0f; // 4y
		return Quaternion((r12 + r21) / s, 0.25f * s, (r23 + r32) / s, (r31 - r13) / s).Normalized();
	}
	const auto s = Mathf::Sqrt(1.0f + r33 - r11 - r22) * 2.0f; // 4z
	return Quaternion((r31 + r13) / s, (r23 + r32) / s, 0.25f * s, (r12 - r21) / s).Normalized();
}


Vector3 ou::Matrix4x4::GetLossyScale() const {
	return Vector3(Vector3(m11, m12, m13).Length(),
				   Vector3(m21, m22, m23).Length(),
				   Vector3(m31, m32, m33).Length());
}


float ou::Matrix4x4::Determinant() const {
	return MinorTerms(m).Determinant();
}


Matrix4x4 ou::Matrix4x4::Inverse() const {
	const MinorTerms t(m);
	const auto det = t.Determinant();
	if (Mathf::Abs(det) < Mathf::Epsilon) { return Matrix4x4::Identity; }

	Matrix4x4 result = {
		+m[1][1] * t.c5 - m[1][2] * t.c4 + m[1][3] * t.c3,
		-m[0][1] * t.c5 + m[0][2] * t.c4 - m[0][3] * t.c3,
		+m[3][1] * t.s5 - m[3][2] * t.s4 + m[3][3] * t.s3,
		-m[2][1] * t.s5 + m[2][2] * t.s4 - m[2][3] * t.s3,

		-m[1][0] * t.c5 + m[1][2] * t.c2 - m[1][3] * t.c1,
		+m[0][0] * t.c5 - m[0][2] * t.c2 + m[0][3] * t.c1,
		-m[3][0] * t.s5 + m[3][2] * t.s2 - m[3][3] * t.s1,
		+m[2][0] * t.s5 - m[2][2] * t.s2 + m[2][3] * t.s1,

		+m[1][0] * t.c4 - m[1][1] * t.c2 + m[1][3] * t.c0,
		-m[0][0] * t.c4 + m[0][1] * t.c2 - m[0][3] * t.c0,
		+m[3][0] * t.s4 - m[3][1] * t.s2 + m[3][3] * t.s0,
		-m[2][0] * t.s4 + m[2][1] * t.s2 - m[2][3] * t.s0,

		-m[1][0] * t.c3 + m[1][1] * t.c1 - m[1][2] * t.c0,
		+m[0][0] * t.c3 - m[0][1] * t.c1 + m[0][2] * t.c0,
		-m[3][0] * t.s3 + m[3][1] * t.s1 - m[3][2] * t.s0,
		+m[2][0] * t.s3 - m[2][1] * t.s1 + m[2][2] * t.s0};

	return result * (1.0f / det);
}


Matrix4x4 ou::Matrix4x4::Transpose() const {
	return {
		m11, m21, m31, m41,
		m12, m22, m32, m42,
		m13, m23, m33, m43,
		m14, m24, m34, m44};
}


Vector3 ou::Matrix4x4::MultiplyPoint(const Vector3 &point) const {
	const auto x = point.x * m11 + point.y * m21 + point.z * m31 + m41;
	const auto y = point.x * m12 + point.y * m22 + point.z * m32 + m42;
	const auto z = point.x * m13 + point.y * m23 + point.z * m33 + m43;
	const auto w = point.x * m14 + point.y * m24 + point.z * m34 + m44;

	return {x / w, y / w, z / w};
}


Vector3 ou::Matrix4x4::MultiplyPoint3x4(const Vector3 &point) const {
	return {
		point.x * m11 + point.y * m21 + point.z * m31 + m41,
		point.x * m12 + point.y * m22 + point.z * m32 + m42,
		point.x * m13 + point.y * m23 + point.z * m33 + m43};
}


Vector3 ou::Matrix4x4::MultiplyVector(const Vector3 &vector) const {
	return {
		vector.x * m11 + vector.y * m21 + vector.z * m31,
		vector.x * m12 + vector.y * m22 + vector.z * m32,
		vector.x * m13 + vector.y * m23 + vector.z * m33};
}


bool ou::Matrix4x4::IsIdentity() const {
	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++) {
			const auto expected = (row == column) ? 1.0f : 0.0f;
			if ((Mathf::Abs(m[row][column] - expected) > Mathf::Epsilon)) {
				return false;
			}
		}
	}
	return true;
}


std::string ou::Matrix4x4::ToString() const {
	return std::format(
		"({:.2f}, {:.2f}, {:.2f}, {:.2f})\n({:.2f}, {:.2f}, {:.2f}, {:.2f})\n({:.2f}, {:.2f}, {:.2f}, {:.2f})\n({:.2f}, {:.2f}, {:.2f}, {:.2f})",
		m11, m12, m13, m14,
		m21, m22, m23, m24,
		m31, m32, m33, m34,
		m41, m42, m43, m44);
}


std::string ou::Matrix4x4::ToString(std::string_view format) const {
	const auto fmt = std::format(
		"({{:{0}}}, {{:{0}}}, {{:{0}}}, {{:{0}}})\n({{:{0}}}, {{:{0}}}, {{:{0}}}, {{:{0}}})\n({{:{0}}}, {{:{0}}}, {{:{0}}}, {{:{0}}})\n({{:{0}}}, {{:{0}}}, {{:{0}}}, {{:{0}}})", format);
	return std::vformat(fmt, std::make_format_args(m11, m12, m13, m14,
												   m21, m22, m23, m24,
												   m31, m32, m33, m34,
												   m41, m42, m43, m44));
}


Matrix4x4 ou::Matrix4x4::Translate(const Vector3 &v) {
	return {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		v.x, v.y, v.z, 1.0f};
}


Matrix4x4 ou::Matrix4x4::Scale(const Vector3 &v) {
	return {
		v.x, 0.0f, 0.0f, 0.0f,
		0.0f, v.y, 0.0f, 0.0f,
		0.0f, 0.0f, v.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f};
}


Matrix4x4 ou::Matrix4x4::Rotate(const Vector3 &v) {
	const float sx = Mathf::Sin(v.x);
	const float cx = Mathf::Cos(v.x);
	const float sy = Mathf::Sin(v.y);
	const float cy = Mathf::Cos(v.y);
	const float sz = Mathf::Sin(v.z);
	const float cz = Mathf::Cos(v.z);

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


Matrix4x4 ou::Matrix4x4::Rotate(const Quaternion &q) {
	const auto n = q.Normalized();
	const auto xx = n.x * n.x;
	const auto yy = n.y * n.y;
	const auto zz = n.z * n.z;
	const auto xy = n.x * n.y;
	const auto xz = n.x * n.z;
	const auto yz = n.y * n.z;
	const auto wx = n.w * n.x;
	const auto wy = n.w * n.y;
	const auto wz = n.w * n.z;

	return {
		1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f,
		2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx), 0.0f,
		2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy), 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f};
}


Matrix4x4 ou::Matrix4x4::SRT(const Vector3 &s, const Vector3 &r, const Vector3 &t) {
	return Scale(s) * Rotate(r) * Translate(t);
}


Matrix4x4 ou::Matrix4x4::SRT(const Vector3 &s, const Quaternion &r, const Vector3 &t) {
	return Scale(s) * Rotate(r) * Translate(t);
}


Matrix4x4 ou::Matrix4x4::Perspective(float fovY, float aspect, float nearZ, float farZ) {
	const auto yScale = 1.0f / Mathf::Tan(fovY * 0.5f);
	const auto xScale = yScale / aspect;
	const auto range = farZ / (farZ - nearZ);

	return {xScale, 0.0f, 0.0f, 0.0f,
			0.0f, yScale, 0.0f, 0.0f,
			0.0f, 0.0f, range, 1.0f,
			0.0f, 0.0f, -range * nearZ, 0.0f};
}


Matrix4x4 ou::Matrix4x4::Ortho(float width, float height, float nearZ, float farZ) {
	const auto range = 1.0f / (farZ - nearZ);

	return {2.0f / width, 0.0f, 0.0f, 0.0f,
			0.0f, 2.0f / height, 0.0f, 0.0f,
			0.0f, 0.0f, range, 0.0f,
			0.0f, 0.0f, -range * nearZ, 1.0f};
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


Matrix4x4 &ou::Matrix4x4::operator*=(const Matrix4x4 &matrix) {
	return *this = *this * matrix;
}


Matrix4x4 &ou::Matrix4x4::operator*=(float scalar) {
	return *this = *this * scalar;
}


float &ou::Matrix4x4::operator()(int row, int column) {
	ASSERT(0 <= row && row < 4 && 0 <= column && column < 4,
		   "範囲外のインデックスです: [{}] [{}]", row, column);
	return m[row][column];
}


float ou::Matrix4x4::operator()(int row, int column) const {
	ASSERT(0 <= row && row < 4 && 0 <= column && column < 4,
		   "範囲外のインデックスです: [{}] [{}]", row, column);
	return m[row][column];
}
