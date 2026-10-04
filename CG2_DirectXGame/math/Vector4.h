#pragma once

#include "Math/Vector2.h"
#include "Math/Vector3.h"

#include <string>


namespace ou {

struct Vector4 {
public: // メンバ変数

	float x;
	float y;
	float z;
	float w;


public: // コンストラクタ

	Vector4() = default;

	Vector4(float x, float y, float z, float w);

	Vector4(const Vector2 &vec, float z, float w);

	Vector4(const Vector3 &vec, float w);


public: // メンバ関数

	Vector2 XY() const;

	Vector2 YZ() const;

	Vector2 ZX() const;

	Vector3 XYZ() const;

	Vector3 ZYX() const;

	float Length() const;

	float Length2() const;

	void Normalize();

	std::string ToString() const;


public: // 静的メンバ変数

	static const Vector4 Zero;

	static const Vector4 One;

	// TODO: Infinite +/- を追加する


public: // 静的メンバ関数

	static float Distance(const Vector4 &v1, const Vector4 &v2);

	static Vector4 Normalize(const Vector4 &v);

	static float Dot(const Vector4 &v1, const Vector4 &v2);

	static Vector4 Min(const Vector4 &v1, const Vector4 &v2);

	static Vector4 Max(const Vector4 &v1, const Vector4 &v2);

	// TODO: Projectを追加


public: // オペレータ

	friend Vector4 operator+(const Vector4 &v1, const Vector4 &v2);
	friend Vector4 operator-(const Vector4 &v1, const Vector4 &v2);
	friend Vector4 operator*(const Vector4 &v1, float v2);
	friend Vector4 operator*(float v1, const Vector4 &v2);
	friend Vector4 operator/(const Vector4 &v1, float v2);
	friend Vector4 operator/(float v1, const Vector4 &v2);

	Vector4 &operator+=(const Vector4 &v);
	Vector4 &operator-=(const Vector4 &v);
	Vector4 &operator*=(float v);
	Vector4 &operator/=(float v);

	float &operator[](int index);
	float operator[](int index) const;
	Vector4 operator-() const;
};

} // namespace ou