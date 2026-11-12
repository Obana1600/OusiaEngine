#pragma once

#include <string>
#include <string_view>


namespace ou {

// 前方宣言
struct Vector3;

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

	// 指定されたベクトルが完全に等しいか
	bool Equals(const Vector2 &other) const;

	// ベクトルの長さを返す
	float Length() const;

	// ベクトルの 2 乗の長さを返す
	float LengthSquared() const;

	// 正規化したベクトルを作成し、それを返す
	Vector2 Normalized() const;

	// ベクトルの値をフォーマットされた文字列として返す
	std::string ToString() const;
	std::string ToString(std::string_view format) const;


public: // TODO: API を増やす / コメント書く

	static Vector2 Normalize(Vector2 &v);

	static float Distance(const Vector2 &a, const Vector2 &b);

	static float Dot(const Vector2 &a, const Vector2 &b);

	static float Cross(const Vector2 &a, const Vector2 &b);

	static Vector2 Min(const Vector2 &a, const Vector2 &b);

	static Vector2 Max(const Vector2 &a, const Vector2 &b);

	static Vector2 Scale(const Vector2 &a, const Vector2 &b);

	static Vector2 Lerp(const Vector2 &a, const Vector2 &b, float t);
	static Vector2 LerpUnclamped(const Vector2 &a, const Vector2 &b, float t);


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

	operator Vector3() const;
};

} // namespace ou
