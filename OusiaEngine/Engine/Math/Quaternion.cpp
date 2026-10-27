#include "Quaternion.h"
#include "Diagnostics/Assert.h"
#include "Math/Mathf.h"

#include <format>

using namespace ou;


const ou::Quaternion ou::Quaternion::Identity = {0.0f, 0.0f, 0.0f, 1.0f};


ou::Quaternion::Quaternion(float x, float y, float z, float w)
	: x(x),
	  y(y),
	  z(z),
	  w(w) {}


ou::Quaternion::Quaternion(const Vector3 &axis, float angle)
	: Quaternion(AngleAxis(axis, angle)) {}


float ou::Quaternion::Length() const {
	return Mathf::Sqrt((x * x) + (y * y) + (z * z) + (w * w));
}


float ou::Quaternion::LengthSquared() const {
	return (x * x) + (y * y) + (z * z) + (w * w);
}


Quaternion ou::Quaternion::Normalize() const {
	float len = Length();
	if (len < Mathf::Epsilon) {
		return Identity;
	}

	Quaternion q{};
	q.x = x / len;
	q.y = y / len;
	q.z = z / len;
	q.w = w / len;
	return q;
}


Vector3 ou::Quaternion::EulerAngles() const {
	const auto n = Normalize();

	const auto m11 = 1.0f - 2.0f * (n.y * n.y + n.z * n.z);
	const auto m12 = 2.0f * (n.x + n.y + n.w * n.z);
	const auto m13 = 2.0f * (n.x * n.z - n.w * n.y);
	const auto m22 = 1.0f - 2.0f * (n.x * n.x + n.z * n.z);
	const auto m23 = 2.0f * (n.y * n.z + n.w * n.x);
	const auto m32 = 2.0f * (n.y * n.z - n.w * n.x);
	const auto m33 = 1.0f - 2.0f * (n.x * n.x + n.y * n.y);

	const auto sinY = Mathf::ClampN1P1(-m13);
	const auto angleY = Mathf::Asin(sinY);

	if (Mathf::Abs(sinY) > 1.0f - Mathf::Epsilon) {
		return {Mathf::Atan2(-m32, m22), angleY, 0.0f};
	}
	return {Mathf::Atan2(m23, m33), angleY, Mathf::Atan2(m12, m11)};
}


void ou::Quaternion::ToAngleAxis(Vector3 *outAxis, float *outAngle) const {
	const auto cw = Mathf::ClampN1P1(w);
	const float angle = 2.0f * Mathf::Acos(cw);
	const float s = Mathf::Sin(angle * 0.5f);

	Vector3 axis{};
	if (s > Mathf::Epsilon) {
		axis = {x / s, y / s, z / s};
	} else {
		axis = Vector3::Right;
	}

	if (outAxis) { *outAxis = axis; }
	if (outAngle) { *outAngle = angle; }
}


std::string ou::Quaternion::ToString() const {
	return std::format("({:.2f}, {:.2f}, {:.2f}, {:.2f})", x, y, z, w);
}


std::string ou::Quaternion::ToString(std::string_view format) const {
	const auto fmt = std::format("({{:{0}}}, {{:{0}}}, {{:{0}}}, {{:{0}}})", format);
	return std::vformat(fmt, std::make_format_args(x, y, z, w));
}


float ou::Quaternion::Angle(const Quaternion &a, const Quaternion &b) {
	const auto dot = Mathf::Abs(Dot(a, b));
	return 2.0f * Mathf::Acos(Mathf::Min(dot, 1.0f));
}


Quaternion ou::Quaternion::AngleAxis(const Vector3 &axis, float angle) {
	const auto n = axis.Normalize();
	const auto s = Mathf::Sin(angle * 0.5f);
	return {n.x * s, n.y * s, n.z * s, Mathf::Cos(angle * 0.5f)};
}


float ou::Quaternion::Dot(const Quaternion &a, const Quaternion &b) {
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
}


Quaternion ou::Quaternion::Euler(float x, float y, float z) {
	const auto cx = Mathf::Cos(x * 0.5f);
	const auto cy = Mathf::Cos(y * 0.5f);
	const auto cz = Mathf::Cos(z * 0.5f);
	const auto sx = Mathf::Sin(x * 0.5f);
	const auto sy = Mathf::Sin(y * 0.5f);
	const auto sz = Mathf::Sin(z * 0.5f);

	return {sx * cy * cz - cx * sy * sz,
			cx * sy * cz + sx * cy * sz,
			cx * cy * sz - sx * sy * cz,
			cx * cy * cz + sx * sy * sz};
}


Quaternion ou::Quaternion::Euler(const Vector3 &eulerAngles) {
	return Euler(eulerAngles.x, eulerAngles.y, eulerAngles.z);
}


Quaternion ou::Quaternion::Inverse(const Quaternion &q) {
	const auto len2 = q.LengthSquared();
	ASSERT(len2 > 0.0f, "長さの 2 乗が 0 のクォータニオンです");
	return {-q.x / len2, -q.y / len2, -q.z / len2, q.w / len2};
}


Quaternion ou::Quaternion::Normalize(Quaternion &q) {
	return q = q.Normalize();
}


namespace ou {

Quaternion operator*(const Quaternion &a, const Quaternion &b) {
	return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
			a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
			a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
			a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}


Vector3 operator*(const Quaternion &rotation, const Vector3 &point) {
	const auto x2 = rotation.x * 2.0f;
	const auto y2 = rotation.y * 2.0f;
	const auto z2 = rotation.z * 2.0f;

	const auto xx = rotation.x * x2;
	const auto yy = rotation.y * y2;
	const auto zz = rotation.z * z2;
	const auto xy = rotation.x * y2;
	const auto xz = rotation.x * z2;
	const auto yz = rotation.y * z2;
	const auto wx = rotation.w * x2;
	const auto wy = rotation.w * y2;
	const auto wz = rotation.w * z2;

	return {(1.0f - (yy + zz)) * point.x + (xy - wz) * point.y + (xz + wy) * point.z,
			(xy + wz) * point.x + (1.0f - (xx + zz)) * point.y + (yz - wx) * point.z,
			(xz - wy) * point.x + (yz + wx) * point.y + (1.0f - (xx + yy)) * point.z};
}


bool operator==(const Quaternion &a, const Quaternion &b) {
	return Quaternion::Dot(a, b) > 1.0f - Mathf::Epsilon;
}

} // namespace ou


Quaternion &ou::Quaternion::operator*=(const Quaternion &q) {
	return *this = *this * q;
}


float &ou::Quaternion::operator[](int index) {
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


float ou::Quaternion::operator[](int index) const {
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
