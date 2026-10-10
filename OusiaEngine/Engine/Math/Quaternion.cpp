#include "Quaternion.h"
#include "Math/Mathf.h"

#include <algorithm>
#include <cassert>
#include <cmath>


const ou::Quaternion ou::Quaternion::Identity = {0.0f, 0.0f, 0.0f, 1.0f};


ou::Quaternion::Quaternion(float x, float y, float z, float w)
	: x(x),
	  y(y),
	  z(z),
	  w(w) {}


ou::Quaternion::Quaternion(const Vector3 &axis, float r)
	: Quaternion(AngleAxis(axis, r)) {}


float ou::Quaternion::Length() const {
	return std::sqrt(Length2());
}


float ou::Quaternion::Length2() const {
	return Dot(*this, *this);
}


void ou::Quaternion::Normalize() {
	float length = Length();
	if (length == 0.0f) { return; }
	x /= length;
	y /= length;
	z /= length;
	w /= length;
}


ou::Vector3 ou::Quaternion::ToEulerAngles() const {
	const Quaternion q = Normalize(*this);

	// Matrix4x4::Rotate と同じ Rx * Ry * Rz (行ベクトル) の回転行列の成分から逆算する
	const float m11 = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
	const float m12 = 2.0f * (q.x * q.y + q.w * q.z);
	const float m13 = 2.0f * (q.x * q.z - q.w * q.y);
	const float m22 = 1.0f - 2.0f * (q.x * q.x + q.z * q.z);
	const float m23 = 2.0f * (q.y * q.z + q.w * q.x);
	const float m32 = 2.0f * (q.y * q.z - q.w * q.x);
	const float m33 = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);

	const float sinY = std::clamp(-m13, -1.0f, 1.0f);
	const float angleY = std::asin(sinY);

	// ジンバルロック時は x と z の回転を区別できないので z を 0 とみなす
	constexpr float kGimbalLockThreshold = 0.9999f;
	if (std::abs(sinY) < kGimbalLockThreshold) {
		return {std::atan2(m23, m33), angleY, std::atan2(m12, m11)};
	}
	return {std::atan2(-m32, m22), angleY, 0.0f};
}


void ou::Quaternion::ToAxisAngle(Vector3 *axis, float *r) const {
	assert(axis && r);

	const Quaternion q = Normalize(*this);
	const float cosHalf = std::clamp(q.w, -1.0f, 1.0f);
	*r = 2.0f * std::acos(cosHalf);

	const float sinHalf = std::sqrt(1.0f - cosHalf * cosHalf);
	if (sinHalf < Mathf::Epsilon) {
		// 回転していないので軸はどれでもよい
		*axis = Vector3::Right;
		return;
	}
	*axis = {q.x / sinHalf, q.y / sinHalf, q.z / sinHalf};
}


bool ou::Quaternion::IsIdentity() const {
	// q と -q は同じ回転を表すので w の符号は問わない
	return std::abs(x) <= Mathf::Epsilon &&
		   std::abs(y) <= Mathf::Epsilon &&
		   std::abs(z) <= Mathf::Epsilon &&
		   std::abs(std::abs(w) - 1.0f) <= Mathf::Epsilon;
}


ou::Quaternion ou::Quaternion::Normalize(const Quaternion &q) {
	auto result = q;
	result.Normalize();
	return result;
}


float ou::Quaternion::Dot(const Quaternion &q1, const Quaternion &q2) {
	return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}


float ou::Quaternion::Angle(const Quaternion &q1, const Quaternion &q2) {
	const float dot = std::min(std::abs(Dot(q1, q2)), 1.0f);
	return 2.0f * std::acos(dot);
}


ou::Quaternion ou::Quaternion::AngleAxis(const Vector3 &axis, float angle) {
	const Vector3 n = Vector3::Normalize(axis);
	const float s = std::sin(angle * 0.5f);
	return {n.x * s, n.y * s, n.z * s, std::cos(angle * 0.5f)};
}


ou::Quaternion ou::Quaternion::Inverse(const Quaternion &q) {
	const float length2 = q.Length2();
	assert(length2 > 0.0f);
	return {-q.x / length2, -q.y / length2, -q.z / length2, q.w / length2};
}


namespace ou {

Quaternion operator*(const Quaternion &q1, const Quaternion &q2) {
	// q1 を先に適用するため、ハミルトン積 q2 * q1 を計算する
	const Quaternion &a = q2;
	const Quaternion &b = q1;
	return {
		a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
		a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
		a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
		a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
	};
}

} // namespace ou


ou::Quaternion &ou::Quaternion::operator*=(const Quaternion &q) {
	return *this = *this * q;
}


float &ou::Quaternion::operator[](int index) {
	assert(0 <= index && index < 4);
	return (&x)[index];
}


float ou::Quaternion::operator[](int index) const {
	assert(0 <= index && index < 4);
	return (&x)[index];
}
