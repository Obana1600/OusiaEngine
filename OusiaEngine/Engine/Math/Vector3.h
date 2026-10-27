#pragma once

#include "Math/Vector2.h"

#include <string>
#include <string_view>


namespace ou {

struct Vector3 {
public:

	float x;
	float y;
	float z;


public:

	// Vector3(0, 0, 0)
	static const Vector3 Zero;

	// Vector3(0, 0, 0)
	static const Vector3 One;

	// Vector3(-1, 0, 0)
	static const Vector3 Left;

	// Vector3(1, 0, 0)
	static const Vector3 Right;

	// Vector3(0, 1, 0)
	static const Vector3 Up;

	// Vector3(0, -1, 0)
	static const Vector3 Down;

	// Vector3(0, 0, 1)
	static const Vector3 Forward;

	// Vector3(0, 0, -1)
	static const Vector3 Back;

	// Vector3(+INF, +INF, +INF)
	static const Vector3 PositiveInfinity;

	// Vector3(-INF, -INF, -INF)
	static const Vector3 NegativeInfinity;


public:

	// デフォルトコンストラクタ
	Vector3();

	// x, y, z 成分で作成する
	Vector3(float x, float y, float z);

	// Vector2, z 成分で作成する
	Vector3(const Vector2 &vec, float z);


public:

	// Vector2(x, y)
	Vector2 XY() const;

	// Vector2(y, z)
	Vector2 YZ() const;

	// Vector2(z, x)
	Vector2 ZX() const;

	// 指定されたベクトルが完全に等しいか
	bool Equals(const Vector3 &other) const;

	// ベクトルの長さを返す
	float Length() const;

	// ベクトルの 2 乗の長さを返す
	float LengthSquared() const;

	// 正規化したベクトルを作成し、それを返す
	Vector3 Normalize() const;

	// ベクトルの値をフォーマットされた文字列として返す
	std::string ToString() const;
	std::string ToString(std::string_view format) const;


public: // TODO: API の追加 / コメント書く

	static float Distance(const Vector3 &v1, const Vector3 &v2);

	static Vector3 Normalize(Vector3 &v);

	static float Dot(const Vector3 &v1, const Vector3 &v2);

	static Vector3 Cross(const Vector3 &v1, const Vector3 &v2);

	static Vector3 Min(const Vector3 &v1, const Vector3 &v2);

	static Vector3 Max(const Vector3 &v1, const Vector3 &v2);

	static Vector3 Lerp(const Vector3 &a, const Vector3 &b, float t);

	static Vector3 LerpUnclamped(const Vector3 &a, const Vector3 &b, float t);

	static Vector3 Scale(const Vector3 &a, const Vector3 &b);

	// TODO: Projectを追加


public:

	friend Vector3 operator+(const Vector3 &v1, const Vector3 &v2);
	friend Vector3 operator-(const Vector3 &v1, const Vector3 &v2);
	friend Vector3 operator*(const Vector3 &v1, float v2);
	friend Vector3 operator*(float v1, const Vector3 &v2);
	friend Vector3 operator/(const Vector3 &v1, float v2);
	friend Vector3 operator/(float v1, const Vector3 &v2);
	friend bool operator==(const Vector3 &a, const Vector3 &b);

	Vector3 &operator+=(const Vector3 &v);
	Vector3 &operator-=(const Vector3 &v);
	Vector3 &operator*=(float v);
	Vector3 &operator/=(float v);

	float &operator[](int index);
	float operator[](int index) const;
	Vector3 operator-() const;
};

} // namespace ou