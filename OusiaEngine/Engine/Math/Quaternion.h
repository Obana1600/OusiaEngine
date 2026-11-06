#pragma once

#include "Math/Vector3.h"

#include <string>
#include <string_view>


namespace ou {

struct Quaternion {
public: // メンバ変数

	float x;
	float y;
	float z;
	float w;


public: // 静的メンバ変数

	// 単位行列
	static const Quaternion Identity;


public: // コンストラクタ

	Quaternion() = default;

	Quaternion(float x, float y, float z, float w);

	Quaternion(const Vector3 &axis, float angle);


public: // メンバ関数

	float Length() const;

	float LengthSquared() const;

	Quaternion Normalized() const;

	Vector3 EulerAngles() const;

	void ToAngleAxis(Vector3 *outAxis, float *outAngle) const;

	std::string ToString() const;
	std::string ToString(std::string_view format) const;


public: // 静的メンバ関数

	static float Angle(const Quaternion &a, const Quaternion &b);

	static Quaternion AngleAxis(const Vector3 &axis, float angle);

	static float Dot(const Quaternion &a, const Quaternion &b);

	static Quaternion Euler(float x, float y, float z);
	static Quaternion Euler(const Vector3 &eulerAngles);

	static Quaternion Inverse(const Quaternion &q);

	static Quaternion Normalize(Quaternion &q);

	static Quaternion Slerp(const Quaternion &a, const Quaternion &b, float t);
	static Quaternion SlerpUnclamped(const Quaternion &a, const Quaternion &b, float t);


public: // オペレータ

	friend Quaternion operator*(const Quaternion &a, const Quaternion &b);
	friend Vector3 operator*(const Quaternion &rotation, const Vector3 &point);
	friend bool operator==(const Quaternion &a, const Quaternion &b);

	Quaternion &operator*=(const Quaternion &q);
	float &operator[](int index);
	float operator[](int index) const;
};

} // namespace ou
