#pragma once

#include <string>


namespace ou {

struct Vector2 {
public: // メンバ変数

	float x;
	float y;


public: // コンストラクタ

	Vector2() = default;

	Vector2(float x, float y);


public: // メンバ関数

	float Length() const;

	float Length2() const;

	void Normalize();

	std::string ToString() const;


public: // 静的メンバ変数

	static const Vector2 Zero;

	static const Vector2 One;

	static const Vector2 Up;

	static const Vector2 Down;

	static const Vector2 Left;

	static const Vector2 Right;

	// TODO: Infinite +/- を追加する


public: // 静的メンバ関数

	static Vector2 Normalize(const Vector2 &v);

	static float Distance(const Vector2 &v1, const Vector2 &v2);

	static float Dot(const Vector2 &v1, const Vector2 &v2);

	static float Cross(const Vector2 &v1, const Vector2 &v2);

	static Vector2 Min(const Vector2 &v1, const Vector2 &v2);

	static Vector2 Max(const Vector2 &v1, const Vector2 &v2);

	static Vector2 Scale(const Vector2 &v1, const Vector2 &v2);


public: // オペレータ

	friend Vector2 operator+(const Vector2 &v1, const Vector2 &v2);
	friend Vector2 operator-(const Vector2 &v1, const Vector2 &v2);
	friend Vector2 operator*(const Vector2 &v1, float v2);
	friend Vector2 operator*(float v1, const Vector2 &v2);
	friend Vector2 operator/(const Vector2 &v1, float v2);
	friend Vector2 operator/(float v1, const Vector2 &v2);

	Vector2 &operator+=(const Vector2 &v);
	Vector2 &operator-=(const Vector2 &v);
	Vector2 &operator*=(float v);
	Vector2 &operator/=(float v);

	float &operator[](int index);
	float operator[](int index) const;
	Vector2 operator-() const;
};

} // namespace ou
