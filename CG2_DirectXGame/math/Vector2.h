#pragma once


struct Vector2 {

	// メンバ変数
	float x;
	float y;


	// 演算子オーバーロード --------------------------------------------------

	Vector2 &operator+=(const Vector2 &other);

	Vector2 &operator-=(const Vector2 &other);

	Vector2 &operator*=(float scalar);

	Vector2 &operator/=(float scalar);

	Vector2 operator+() const;

	Vector2 operator-() const;

	// -------------------------------------------------------------------

};

Vector2 operator+(const Vector2 &v1, const Vector2 &v2);

Vector2 operator-(const Vector2 &v1, const Vector2 &v2);

Vector2 operator*(const Vector2 &v, float scalar);

Vector2 operator*(float scalar, const Vector2 &v);

Vector2 operator/(const Vector2 &v, float scalar);