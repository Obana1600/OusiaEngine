#pragma once

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"


namespace MathUtility {

	// Vector2 -----------------------------------------------------------

	// 内積
	float Dot(const Vector2 &v1, const Vector2 &v2);
	// 外積
	float Cross(const Vector2 &v1, const Vector2 &v2);
	// 長さ（ノルム）
	float Length(const Vector2 &v);
	// 正規化
	Vector2 Normalize(const Vector2 &v);

	// -------------------------------------------------------------------


	// Vector3 -----------------------------------------------------------

	// 内積
	float Dot(const Vector3 &v1, const Vector3 &v2);
	// 外積
	Vector3 Cross(const Vector3 &v1, const Vector3 &v2);
	// 長さ（ノルム）
	float Length(const Vector3 &v);
	// 正規化
	Vector3 Normalize(const Vector3 &v);

	// -------------------------------------------------------------------


	// Vector4 -----------------------------------------------------------

	// -------------------------------------------------------------------


	// Matrix4x4 ---------------------------------------------------------

	Matrix4x4 MakeIdentity4x4(); // 単位行列の作成

	Matrix4x4 Transpose(const Matrix4x4 &m); // 転置行列

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

} // namespace MathUtility