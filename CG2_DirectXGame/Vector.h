#pragma once


struct Vector2 {

	// メンバ変数
	float x;
	float y;

	// デフォルトコンストラクタ
	Vector2()
		: x(0.0f), y(0.0f) {
	}

	//　コンストラクタ
	Vector2(float x, float y)
		: x(x), y(y) {
	}


	// 演算子オーバーロード --------------------------------------------------

	Vector2 &operator+=(const Vector2 &other);

	Vector2 &operator-=(const Vector2 &other);

	Vector2 &operator*=(float scalar);

	Vector2 &operator/=(float scalar);

	Vector2 operator+() const;

	Vector2 operator-() const;

	// -------------------------------------------------------------------

}; // Vector2

Vector2 operator+(const Vector2 &v1, const Vector2 &v2);

Vector2 operator-(const Vector2 &v1, const Vector2 &v2);

Vector2 operator*(const Vector2 &v, float scalar);

Vector2 operator*(float scalar, const Vector2 &v);

Vector2 operator/(const Vector2 &v, float scalar);



struct Vector3 {

	// メンバ変数
	float x;
	float y;
	float z;

	// デフォルトコンストラクタ
	Vector3()
		: x(0.0f)
		, y(0.0f)
		, z(0.0f) {
	}

	// コンストラクタ
	Vector3(float x, float y, float z)
		:x(x), y(y), z(z) {
	}


	// 演算子オーバーロード --------------------------------------------------

	Vector3 &operator+=(const Vector3 &other);

	Vector3 &operator-=(const Vector3 &other);

	Vector3 &operator*=(float scalar);

	Vector3 &operator/=(float scalar);

	Vector3 operator+() const;

	Vector3 operator-() const;

	// -------------------------------------------------------------------

}; // Vector3

Vector3 operator+(const Vector3 &v1, const Vector3 &v2);

Vector3 operator-(const Vector3 &v1, const Vector3 &v2);

Vector3 operator*(const Vector3 &v, float scalar);

Vector3 operator*(float scalar, const Vector3 &v);

Vector3 operator/(const Vector3 &v, float scalar);



struct Vector4 {
	float x;
	float y;
	float z;
	float w;
};


namespace MathUtility {

	// Vector2 -----------------------------------------------------------

	float Dot(const Vector2 &v1, const Vector2 &v2);

	float Cross(const Vector2 &v1, const Vector2 &v2);

	float Length(const Vector2 &v); // 長さ（ノルム）

	Vector2 Normalize(const Vector2 &v); // 正規化

	// -------------------------------------------------------------------


	// Vector3 -----------------------------------------------------------
	
	float Dot(const Vector3 &v1, const Vector3 &v2); // 内積
	
	Vector3 Cross(const Vector3 &v1, const Vector3 &v2); // 外積
	
	float Length(const Vector3 &v); // 長さ（ノルム）

	float LengthSq(const Vector3 &v); // 長さ（ノルム）の2乗
	
	Vector3 Normalize(const Vector3 &v); // 正規化
	
	Vector3 Lerp(const Vector3 &v1, const Vector3 &v2, float t); // 線形補間
	
	Vector3 Bezier(const Vector3 &controlPoint0, const Vector3 &controlPoint1, const Vector3 &controlPoint2, float t); // ベジエ

	// -------------------------------------------------------------------

} // MathUtility