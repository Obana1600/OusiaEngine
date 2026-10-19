#pragma once

#include "Math/Vector3.h"

#include <string>


namespace ou {

struct Quaternion {
public: // メンバ変数

	float x;
	float y;
	float z;
	float w;


public: // 静的メンバ変数

	static const Quaternion Identity;


public: // コンストラクタ

	Quaternion() = default;

	Quaternion(float x, float y, float z, float w);

	Quaternion(const Vector3 &axis, float angle);


public: // メンバ関数

	float Length() const;

	float LengthSquared() const;

	Quaternion Normalize() const;

	Vector3 EulerAngles() const;

	void ToAngleAxis(Vector3 *outAxis, float *outAngle) const;

	std::string ToString() const;


public: // 静的メンバ関数

	static float Angle(const Quaternion &a, const Quaternion &b);

	static Quaternion AngleAxis(const Vector3 &axis, float angle);

	static float Dot(const Quaternion &a, const Quaternion &b);

	static Quaternion Euler(float x, float y, float z);
	static Quaternion Euler(const Vector3 &eulerAngles);

	static Quaternion Inverse(const Quaternion &q);

	static Quaternion Normalize(Quaternion &q);


public: // オペレータ

	friend Quaternion operator*(const Quaternion &a, const Quaternion &b);
	friend bool operator==(const Quaternion &a, const Quaternion &b);

	Quaternion &operator*=(const Quaternion &q);
	float &operator[](int index);
	float operator[](int index) const;
};

} // namespace ou
