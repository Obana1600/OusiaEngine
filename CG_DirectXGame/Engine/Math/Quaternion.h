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


public: // コンストラクタ

	Quaternion() = default;

	Quaternion(float x, float y, float z, float w);

	Quaternion(const Vector3 &axis, float r);


public: // メンバ関数

	float Length() const;

	float Length2() const;

	void Normalize();

	Vector3 ToEulerAngles() const;

	void ToAxisAngle(Vector3 *axis, float *r) const;

	bool IsIdentity() const;


public: // 静的メンバ変数

	static const Quaternion Identity;


public: // 静的メンバ関数

	static Quaternion Normalize(const Quaternion &q);

	static float Dot(const Quaternion &q1, const Quaternion &q2);

	static float Angle(const Quaternion &q1, const Quaternion &q2);

	static Quaternion AngleAxis(const Vector3 &axis, float angle);

	static Quaternion Inverse(const Quaternion &q);


public: // オペレータ

	// TODO: 加算減算も追加していいかも

	friend Quaternion operator*(const Quaternion &q1, const Quaternion &q2);

	Quaternion &operator*=(const Quaternion &q);
	float &operator[](int index);
	float operator[](int index) const;
};

} // namespace ou