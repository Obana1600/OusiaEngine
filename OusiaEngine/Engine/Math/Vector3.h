#pragma once

#include "Math/Vector2.h"

#include <string>


namespace ou {

struct Vector3 {
public: // メンバ変数

	float x;
	float y;
	float z;


public: // コンストラクタ

	Vector3() = default;

	Vector3(float x, float y, float z);

	Vector3(const Vector2 &vec, float z);


public: // メンバ関数

	Vector2 XY() const;

	Vector2 YZ() const;

	Vector2 ZX() const;

	float Length() const;

	float Length2() const;

	void Normalize();

	std::string ToString() const;


public: // 静的メンバ変数

	static const Vector3 Zero;

	static const Vector3 One;

	static const Vector3 Up;

	static const Vector3 Down;

	static const Vector3 Left;

	static const Vector3 Right;

	static const Vector3 Forward;

	static const Vector3 Back;

	// TODO: Infinite +/- を追加する


public: // 静的メンバ関数

	static float Distance(const Vector3 &v1, const Vector3 &v2);

	static Vector3 Normalize(const Vector3 &v);

	static float Dot(const Vector3 &v1, const Vector3 &v2);

	static Vector3 Cross(const Vector3 &v1, const Vector3 &v2);

	static Vector3 Min(const Vector3 &v1, const Vector3 &v2);

	static Vector3 Max(const Vector3 &v1, const Vector3 &v2);

	// TODO: Projectを追加


public: // オペレータ

	friend Vector3 operator+(const Vector3 &v1, const Vector3 &v2);
	friend Vector3 operator-(const Vector3 &v1, const Vector3 &v2);
	friend Vector3 operator*(const Vector3 &v1, float v2);
	friend Vector3 operator*(float v1, const Vector3 &v2);
	friend Vector3 operator/(const Vector3 &v1, float v2);
	friend Vector3 operator/(float v1, const Vector3 &v2);

	Vector3 &operator+=(const Vector3 &v);
	Vector3 &operator-=(const Vector3 &v);
	Vector3 &operator*=(float v);
	Vector3 &operator/=(float v);

	float &operator[](int index);
	float operator[](int index) const;
	Vector3 operator-() const;
};

} // namespace ou