#pragma once

#include "Vector.h"


struct Matrix2x2 {
	float m[2][2];
};


struct Matrix3x3 {
	float m[3][3];
};


struct Matrix4x4 {
	float m[4][4];
};



namespace MathUtility {

	// Matrix2x2 ---------------------------------------------------------

	// 行列の加算
	Matrix2x2 Add(const Matrix2x2 &m1, const Matrix2x2 &m2);
	// 行列の減算
	Matrix2x2 Subtract(const Matrix2x2 &m1, const Matrix2x2 &m2);
	// 行列の乗算
	Matrix2x2 Multiply(const Matrix2x2 &m1, const Matrix2x2 &m2);
	// ベクトルと行列の乗算
	Vector2 Multiply(const Vector2 &v, const Matrix2x2 &m);
	// 回転行列の作成
	Matrix2x2 MakeRotateMatrix2x2(float theta);
	// 行列式
	float Determinant(const Matrix2x2 &m);
	// 逆行列
	Matrix2x2 Inverse(const Matrix2x2 &m);
	// 転置行列
	Matrix2x2 Transpose(const Matrix2x2 &m);

	// -------------------------------------------------------------------


	// Matrix3x3 ---------------------------------------------------------

	// 平行移動行列の作成
	Matrix3x3 MakeTranslateMatrix(const Vector2 &translate);
	// 2次元ベクトルから同次座標への変換
	Vector2 Transform(const Vector2 &v, const Matrix3x3 &m);
	// 3x3行列同士の乗算
	Matrix3x3 Multiply(const Matrix3x3 &m1, const Matrix3x3 &m2);
	// 回転行列の作成
	Matrix3x3 MakeRotateMatrix3x3(float theta);
	// 拡大縮小行列の作成
	Matrix3x3 MakeScaleMatrix(const Vector2 &scale);
	// アフィン変換行列の合成
	Matrix3x3 MakeAffineMatrix(const Vector2 &translate, const Vector2 &scale, float rotate);
	// 行列式
	float Determinant(const Matrix3x3 &m);
	float Determinant(float a00, float a01, float a02, float a10, float a11, float a12, float a20, float a21, float a22);
	// 逆行列
	Matrix3x3 Inverse(const Matrix3x3 &m);
	// 転置行列
	Matrix3x3 Transpose(const Matrix3x3 &m);
	// カメラのワールド行列の作成
	Matrix3x3 MakeCameraWorldMatrix(float positionWorldX, float positionWorldY);
	// 正射影行列の作成
	Matrix3x3 MakeOrthographicMatrix(float left, float top, float right, float bottom);
	// ビューポート行列の作成
	Matrix3x3 MakeViewportMatrix(float left, float top, float width, float height);

	// -------------------------------------------------------------------


	// Matrix4x4 ---------------------------------------------------------

	// 行列の加算
	Matrix4x4 Add(const Matrix4x4 &m1, const Matrix4x4 &m2);
	// 行列の減算
	Matrix4x4 Subtract(const Matrix4x4 &m1, const Matrix4x4 &m2);
	// 行列の積
	Matrix4x4 Multiply(const Matrix4x4 &m1, const Matrix4x4 &m2);
	// 行列式
	float Determinant(const Matrix4x4 &m);
	// 余因子行列
	Matrix4x4 Cofactor(const Matrix4x4 &m);
	// 逆行列
	Matrix4x4 Inverse(const Matrix4x4 &m);
	// 転置行列
	Matrix4x4 Transpose(const Matrix4x4 &m);
	// 単位行列の作成
	Matrix4x4 MakeIdentity4x4();
	// 平行移動行列
	Matrix4x4 MakeTranslateMatrix(const Vector3 &translate);
	// 拡大縮小行列
	Matrix4x4 MakeScaleMatrix(const Vector3 &scale);
	// 座標変換
	Vector3 Transform(const Vector3 &v, const Matrix4x4 &m);
	// X軸回転行列
	Matrix4x4 MakeRotateXMatrix(float radian);
	// Y軸回転行列
	Matrix4x4 MakeRotateYMatrix(float radian);
	// Z軸回転行列
	Matrix4x4 MakeRotateZMatrix(float radian);
	// 3次元アフィン変換行列
	Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate, const Vector3 &translate);
	// 透視投影行列
	Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);
	// 正射影行列
	Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);
	// ビューポート変換行列
	Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

	Matrix4x4 operator*(const Matrix4x4 &m1, const Matrix4x4 &m2);

	// -------------------------------------------------------------------

} // MathUtility