#pragma once


struct Vector2 {

	// メンバ変数
	float x;
	float y;

	// デフォルトコンストラクタ「
	Vector2()
		: x(0.0f), y(0.0f) {
	}

	//　コンストラクタ
	Vector2(float x, float y)
		: x(x), y(y) {
	}


	// 演算子オーバーロード --------------------------------------------------

	Vector2 operator+(const Vector2 &other) const {
		return Vector2(x + other.x, y + other.y);
	}

	Vector2 operator-(const Vector2 &other) const {
		return Vector2(x - other.x, y - other.y);
	}

	Vector2 operator*(float scalar) const {
		return Vector2(x * scalar, y * scalar);
	}

	Vector2 operator/(float scalar) const {
		return Vector2(x / scalar, y / scalar);
	}

	Vector2 &operator+=(const Vector2 &other) {
		x += other.x;
		y += other.y;
		return *this;
	}

	Vector2 &operator-=(const Vector2 &other) {
		x -= other.x;
		y -= other.y;
		return *this;
	}

	Vector2 &operator*=(float scalar) {
		x *= scalar;
		y *= scalar;
		return *this;
	}

	Vector2 &operator/=(float scalar) {
		x /= scalar;
		y /= scalar;
		return *this;
	}

	Vector2 operator-() const {
		return Vector2(-x, -y);
	}

	// -------------------------------------------------------------------

}; // Vector2


inline Vector2 operator*(float scalar, const Vector2 &v) {
	return v * scalar;
}



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

	Vector3 operator+(const Vector3 &other) const {
		return Vector3(x + other.x, y + other.y, z + other.z);
	}

	Vector3 operator-(const Vector3 &other) const {
		return Vector3(x - other.x, y - other.y, z - other.z);
	}

	Vector3 operator*(float scalar) const {
		return Vector3(x * scalar, y * scalar, z * scalar);
	}

	Vector3 operator/(float scalar) const {
		return Vector3(x / scalar, y / scalar, z / scalar);
	}

	Vector3 &operator+=(const Vector3 &other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	Vector3 &operator-=(const Vector3 &other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	Vector3 &operator*=(float scalar) {
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}

	Vector3 &operator/=(float scalar) {
		x /= scalar;
		y /= scalar;
		z /= scalar;
		return *this;
	}

	Vector3 operator-() const {
		return Vector3(-x, -y, -z);
	}

	// -------------------------------------------------------------------

}; // Vector3


inline Vector3 operator*(float scalar, const Vector3 &v) {
	return v * scalar;
}



struct Vector4 {

	float x;
	float y;
	float z;
	float w;

};



namespace MathUtility {

	// 加算
	Vector3 Add(const Vector3 &v1, const Vector3 &v2);
	// 減算
	Vector3 Subtract(const Vector3 &v1, const Vector3 &v2);
	// スカラー倍
	Vector3 Multiply(float scalar, const Vector3 &v);
	Vector3 Multiply(const Vector3 &v, float scalar);
	// 内積
	float Dot(const Vector3 &v1, const Vector3 &v2);
	// 外積
	Vector3 Cross(const Vector3 &v1, const Vector3 &v2);
	// 長さ（ノルム）
	float Length(const Vector3 &v);
	// 正規化
	Vector3 Normalize(const Vector3 &v);

} // MathUtility