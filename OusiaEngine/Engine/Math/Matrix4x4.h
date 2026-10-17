#pragma once

#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Quaternion.h"

#include <string>


namespace ou {

struct Matrix4x4 {
public:

#pragma warning(push)
#pragma warning(disable : 4201) // 無名構造体を使う
	union {
		struct {
			float m11, m12, m13, m14;
			float m21, m22, m23, m24;
			float m31, m32, m33, m34;
			float m41, m42, m43, m44;
		};

		float m[4][4];
	};
#pragma warning(pop)


public:

	static const Matrix4x4 Identity;

	static const Matrix4x4 Zero;


public:

	// デフォルトコンストラクタ
	Matrix4x4();

	// 16 のすべての要素を指定して作成する 
	Matrix4x4(float m11, float m12, float m13, float m14,
			  float m21, float m22, float m23, float m24,
			  float m31, float m32, float m33, float m34,
			  float m41, float m42, float m43, float m44);

	// 各行を Vector4 で指定して作成する
	Matrix4x4(const Vector4 &row1, const Vector4 &row2, const Vector4 &row3, const Vector4 &row4);


public: // TODO: API を増やす / コメント書く

	Vector4 GetColumn(int index) const;

	void SetColumn(int index, const Vector4 &v);
	
	Vector4 GetRow(int index) const;

	void SetRow(int index, const Vector4 &v);

	float Determinant() const;

	Matrix4x4 Inverse() const;

	Matrix4x4 Transpose() const;

	Vector3 MultiplyPoint(const Vector3 &point) const;

	Vector3 MultiplyPoint3x4(const Vector3 &point) const;

	Vector3 MultiplyVector(const Vector3 &vector) const;

	bool IsIdentity() const;

	std::string ToString() const;


public: // TODO: API を増やす / コメント書く

	static Matrix4x4 Translate(const Vector3 &v);

	static Matrix4x4 Scale(const Vector3 &v);

	static Matrix4x4 Rotate(const Vector3 &v);
	static Matrix4x4 Rotate(const Quaternion &q);

	static Matrix4x4 SRT(const Vector3 &s, const Vector3 &r, const Vector3 &t);
	static Matrix4x4 SRT(const Vector3 &s, const Quaternion &r, const Vector3 &t);

	static Matrix4x4 Perspective(float fovY, float aspect, float nearZ, float farZ);

	static Matrix4x4 Ortho(float width, float height, float nearZ, float farZ);


public: // オペレータ

	// TODO: Matrix4x4 * float はいらないかも

	friend Matrix4x4 operator*(const Matrix4x4 &m1, const Matrix4x4 &m2);
	friend Matrix4x4 operator*(const Matrix4x4 &m1, float m2);

	Matrix4x4 &operator*=(const Matrix4x4 &m);
	Matrix4x4 &operator*=(float m);
	float &operator()(int row, int column);
	float operator()(int row, int column) const;
};

} // namespace ou