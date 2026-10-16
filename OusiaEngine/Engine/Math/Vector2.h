#pragma once

#include <string>


namespace ou {

struct Vector2 {
public:

	float x;
	float y;


public:

	// Vector2(0, 0)
	static const Vector2 Zero;

	// Vector2(1, 1)
	static const Vector2 One;

	// Vector2(0, 1)
	static const Vector2 Up;

	// Vector2(0, -1)
	static const Vector2 Down;

	// Vector2(-1, 0)
	static const Vector2 Left;

	// Vector2(1, 0)
	static const Vector2 Right;

	// Vector2(+INF, +INF)
	static const Vector2 PositiveInfinity;

	// Vector2(-INF, -INF)
	static const Vector2 NegativeInfinity;


public: 

	// デフォルトコンストラクタ
	Vector2();

	// x, y 成分で作成する
	Vector2(float x, float y);


public:

	// ベクトルの長さを返す
	float Length() const;

	// ベクトルの 2 乗の長さを返す
	float LengthSquared() const;

	// 正規化したベクトルを作成し、それを返す
	Vector2 Normalize() const;

	// ベクトルの値をフォーマットされた文字列として返す
	std::string ToString() const;


public: // 静的メンバ関数

	static Vector2 Normalize(Vector2 &v);

	static float Distance(const Vector2 &a, const Vector2 &b);

	static float Dot(const Vector2 &a, const Vector2 &b);

	static float Cross(const Vector2 &a, const Vector2 &b);

	static Vector2 Min(const Vector2 &a, const Vector2 &b);

	static Vector2 Max(const Vector2 &a, const Vector2 &b);

	static Vector2 Scale(const Vector2 &a, const Vector2 &b);


public: // オペレータ

	friend Vector2 operator+(const Vector2 &v1, const Vector2 &v2);
	friend Vector2 operator-(const Vector2 &v1, const Vector2 &v2);
	friend Vector2 operator*(const Vector2 &v1, float v2);
	friend Vector2 operator*(float v1, const Vector2 &v2);
	friend Vector2 operator/(const Vector2 &v1, float v2);
	friend Vector2 operator/(float v1, const Vector2 &v2);
	friend bool operator==(const Vector2 &v1, const Vector2 &v2);

	Vector2 &operator+=(const Vector2 &v);
	Vector2 &operator-=(const Vector2 &v);
	Vector2 &operator*=(float v);
	Vector2 &operator/=(float v);

	float &operator[](int index);
	float operator[](int index) const;
	Vector2 operator-() const;
};

} // namespace ou
