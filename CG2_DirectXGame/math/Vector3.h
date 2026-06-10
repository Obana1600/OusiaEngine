#pragma once


struct Vector3 {

	// メンバ変数
	float x;
	float y;
	float z;


	// 演算子オーバーロード --------------------------------------------------

	Vector3 &operator+=(const Vector3 &other);

	Vector3 &operator-=(const Vector3 &other);

	Vector3 &operator*=(float scalar);

	Vector3 &operator/=(float scalar);

	Vector3 operator+() const;

	Vector3 operator-() const;

	// -------------------------------------------------------------------

};

Vector3 operator+(const Vector3 &v1, const Vector3 &v2);

Vector3 operator-(const Vector3 &v1, const Vector3 &v2);

Vector3 operator*(const Vector3 &v, float scalar);

Vector3 operator*(float scalar, const Vector3 &v);

Vector3 operator/(const Vector3 &v, float scalar);