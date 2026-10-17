#pragma once

#include "Math/Vector2.h"
#include "Math/Vector3.h"

#include <string>


namespace ou {

struct Vector4 {
public:

	float x;
	float y;
	float z;
	float w;


public:

	// Vector4(0, 0, 0, 0)
	static const Vector4 Zero;

	// Vector4(1, 1, 1, 1)
	static const Vector4 One;

	// Vector4(+INF, +INF, +INF, +INF)
	static const Vector4 PositiveInifinity;

	// Vector4(-INF, -INF, -INF, -INF)
	static const Vector4 NegativeInifinity;


public:

	// デフォルトコンストラクタ
	Vector4();

	// x, y, z, w 成分で作成する
	Vector4(float x, float y, float z, float w);

	// Vector2, z, w 成分で作成する
	Vector4(const Vector2 &vec, float z, float w);

	// Vector3, w 成分で作成する
	Vector4(const Vector3 &vec, float w);

	// Vector2 成分で作成する (z, w は 0.0f)
	Vector4(const Vector2 &v);

	// Vector3 成分で作成する (w は 0.0f)
	Vector4(const Vector3 &v);


public:

	// Vector2(x, y)
	Vector2 XY() const;

	// Vector2(y, z)
	Vector2 YZ() const;

	// Vector2(z, x)
	Vector2 ZX() const;

	// Vector3(x, y, z)
	Vector3 XYZ() const;

	// Vector3(z, y, x)
	Vector3 ZYX() const;

	// 指定されたベクトルが完全に等しいか
	bool Equals(const Vector4 &other) const;

	// ベクトルの長さを返す
	float Length() const;

	// ベクトルの 2 乗の長さを返す
	float LengthSquared() const;

	// 正規化したベクトルを作成し、それを返す
	Vector4 Normalize() const;

	// ベクトルの値をフォーマットされた文字列として返す
	std::string ToString() const;
	// TODO: フォーマット指定のオーバーライド


public: // TODO: API の追加 / コメント書く

	static float Distance(const Vector4 &v1, const Vector4 &v2);

	static Vector4 Normalize(Vector4 &v);

	static float Dot(const Vector4 &v1, const Vector4 &v2);

	static Vector4 Min(const Vector4 &v1, const Vector4 &v2);

	static Vector4 Max(const Vector4 &v1, const Vector4 &v2);

	static Vector4 Lerp(const Vector4 &a, const Vector4 &b, float t);

	static Vector4 LerpUnclamped(const Vector4 &a, const Vector4 &b, float t);

	// TODO: Projectを追加


public: // オペレータ

	friend Vector4 operator+(const Vector4 &v1, const Vector4 &v2);
	friend Vector4 operator-(const Vector4 &v1, const Vector4 &v2);
	friend Vector4 operator*(const Vector4 &v1, float v2);
	friend Vector4 operator*(float v1, const Vector4 &v2);
	friend Vector4 operator/(const Vector4 &v1, float v2);
	friend Vector4 operator/(float v1, const Vector4 &v2);
	friend bool operator==(const Vector4 &a, const Vector4 &b);

	Vector4 &operator+=(const Vector4 &v);
	Vector4 &operator-=(const Vector4 &v);
	Vector4 &operator*=(float v);
	Vector4 &operator/=(float v);

	float &operator[](int index);
	float operator[](int index) const;
	Vector4 operator-() const;

	operator Vector2() const;
	operator Vector3() const;
};

} // namespace ou