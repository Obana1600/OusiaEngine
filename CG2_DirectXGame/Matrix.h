#pragma once

#include "Vector.h"


struct Matrix3x3 {

	// メンバ変数
	float m[3][3];


	// 演算子オーバーロード --------------------------------------------------

	Matrix3x3 &operator+=(const Matrix3x3 &other);

	Matrix3x3 &operator-=(const Matrix3x3 &other);

	Matrix3x3 &operator*=(const Matrix3x3 &other);

	// -------------------------------------------------------------------

}; // Matrix3x3

Matrix3x3 operator+(const Matrix3x3 &m1, const Matrix3x3 &m2);

Matrix3x3 operator-(const Matrix3x3 &m1, const Matrix3x3 &m2);

Matrix3x3 operator*(const Matrix3x3 &m1, const Matrix3x3 &m2);



struct Matrix4x4 {

	// メンバ変数
	float m[4][4];


	// 演算子オーバーロード --------------------------------------------------

	Matrix4x4 &operator+=(const Matrix4x4 &other);

	Matrix4x4 &operator-=(const Matrix4x4 &other);

	Matrix4x4 &operator*=(const Matrix4x4 &other);

	// -------------------------------------------------------------------

}; // Matrix4x4

Matrix4x4 operator+(const Matrix4x4 &m1, const Matrix4x4 &m2);

Matrix4x4 operator-(const Matrix4x4 &m1, const Matrix4x4 &m2);

Matrix4x4 operator*(const Matrix4x4 &m1, const Matrix4x4 &m2);



namespace MathUtility {

	// Matrix3x3 ---------------------------------------------------------

	Matrix3x3 MakeIdentity3x3(); // 単位行列の作成

	Matrix3x3 Transpose(const Matrix3x3 &m); // 転置行列

	float Determinant(float a00, float a01, float a02, float a10, float a11, float a12, float a20, float a21, float a22); // 行列式
	float Determinant(const Matrix3x3 &m); // 行列式

	Matrix3x3 Cofactor(const Matrix3x3 &m); // 余因子行列

	Matrix3x3 Inverse(const Matrix3x3 &m); // 逆行列

	Vector2 Transform(const Vector2 &v, const Matrix3x3 &m); // 同次座標へ変換

	Matrix3x3 MakeTranslateMatrix(const Vector2 &translate); // 平行移動行列

	Matrix3x3 MakeScaleMatrix(const Vector2 &scale); // 拡縮行列

	Matrix3x3 MakeRotateMatrix(float theta); // 回転行列

	Matrix3x3 MakeAffineMatrix(const Vector2 &translate, float rotate, const Vector2 &scale); // アフィン変換行列

	Matrix3x3 MakeOrthographicMatrix(float left, float top, float right, float bottom); // 正射影行列

	Matrix3x3 MakeViewportMatrix(float left, float top, float width, float height); // ビューポート行列

	// -------------------------------------------------------------------


	// Matrix4x4 ---------------------------------------------------------

	Matrix4x4 MakeIdentity4x4(); // 単位行列の作成

	Matrix4x4 Transpose(const Matrix4x4 &m); // 転置行列

	float Determinant(const Matrix4x4 &m); // 行列式

	Matrix4x4 Cofactor(const Matrix4x4 &m); // 余因子行列

	Matrix4x4 Inverse(const Matrix4x4 &m); // 逆行列

	Vector3 Transform(const Vector3 &v, const Matrix4x4 &m); // 同次座標へ変換

	Matrix4x4 MakeTranslateMatrix(const Vector3 &translate); // 平行移動行列

	Matrix4x4 MakeScaleMatrix(const Vector3 &scale); // 拡縮行列

	Matrix4x4 MakeRotateXMatrix(float radian); // x回転行列

	Matrix4x4 MakeRotateYMatrix(float radian); // y回転行列

	Matrix4x4 MakeRotateZMatrix(float radian); // z回転行列

	Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate, const Vector3 &translate); // アフィン変換行列

	Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip); // 透視投影行列

	Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip); // 正射影行列

	Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth); // ビューポート変換行列

	// -------------------------------------------------------------------

} // MathUtility