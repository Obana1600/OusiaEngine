#pragma once


struct Vector4 {

	// メンバ変数
	float x;
	float y;
	float z;
	float w;


	// 演算子オーバーロード --------------------------------------------------

	Vector4 &operator+=(const Vector4 &other);

	Vector4 &operator-=(const Vector4 &other);

	Vector4 &operator*=(float scalar);

	Vector4 &operator/=(float scalar);

	Vector4 operator+() const;

	Vector4 operator-() const;

	// -------------------------------------------------------------------
};

Vector4 operator+(const Vector4 &v1, const Vector4 &v2);

Vector4 operator-(const Vector4 &v1, const Vector4 &v2);

Vector4 operator*(const Vector4 &v, float scalar);

Vector4 operator*(float scalar, const Vector4 &v);

Vector4 operator/(const Vector4 &v, float scalar);