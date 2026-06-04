#include "MathUtility.h"
#include <cmath>
#include <cassert>


// helper func ====================================================================================

namespace {

	// 3x3の行列式
	float Determinant(float a00, float a01, float a02, float a10, float a11, float a12, float a20, float a21, float a22) {
		return a00 * (a11 * a22 - a12 * a21)
			- a10 * (a01 * a22 - a02 * a21)
			+ a20 * (a01 * a12 - a02 * a11);
	}


	// 4x4の行列式
	float Determinant(const Matrix4x4 &m) {
		// [0][0]の小行列式
		float minor0 = Determinant(
			m.m[1][1], m.m[1][2], m.m[1][3],
			m.m[2][1], m.m[2][2], m.m[2][3],
			m.m[3][1], m.m[3][2], m.m[3][3]
		);

		// [1][0]の小行列式
		float minor1 = Determinant(
			m.m[0][1], m.m[0][2], m.m[0][3],
			m.m[2][1], m.m[2][2], m.m[2][3],
			m.m[3][1], m.m[3][2], m.m[3][3]
		);

		// [2][0]の小行列式
		float minor2 = Determinant(
			m.m[0][1], m.m[0][2], m.m[0][3],
			m.m[1][1], m.m[1][2], m.m[1][3],
			m.m[3][1], m.m[3][2], m.m[3][3]
		);

		// [3][0]の小行列式
		float minor3 = Determinant(
			m.m[0][1], m.m[0][2], m.m[0][3],
			m.m[1][1], m.m[1][2], m.m[1][3],
			m.m[2][1], m.m[2][2], m.m[2][3]
		);

		return (m.m[0][0] * minor0) - (m.m[1][0] * minor1) + (m.m[2][0] * minor2) - (m.m[3][0] * minor3);
	}


	// 余因子行列
	Matrix4x4 Cofactor(const Matrix4x4 &m) {
		Matrix4x4 result{};

		// [0][]の余因子
		result.m[0][0] = +Determinant(
			m.m[1][1], m.m[1][2], m.m[1][3],
			m.m[2][1], m.m[2][2], m.m[2][3],
			m.m[3][1], m.m[3][2], m.m[3][3]
		);
		result.m[0][1] = -Determinant(
			m.m[1][0], m.m[1][2], m.m[1][3],
			m.m[2][0], m.m[2][2], m.m[2][3],
			m.m[3][0], m.m[3][2], m.m[3][3]
		);
		result.m[0][2] = +Determinant(
			m.m[1][0], m.m[1][1], m.m[1][3],
			m.m[2][0], m.m[2][1], m.m[2][3],
			m.m[3][0], m.m[3][1], m.m[3][3]
		);
		result.m[0][3] = -Determinant(
			m.m[1][0], m.m[1][1], m.m[1][2],
			m.m[2][0], m.m[2][1], m.m[2][2],
			m.m[3][0], m.m[3][1], m.m[3][2]
		);

		// [1][]の余因子
		result.m[1][0] = -Determinant(
			m.m[0][1], m.m[0][2], m.m[0][3],
			m.m[2][1], m.m[2][2], m.m[2][3],
			m.m[3][1], m.m[3][2], m.m[3][3]
		);
		result.m[1][1] = +Determinant(
			m.m[0][0], m.m[0][2], m.m[0][3],
			m.m[2][0], m.m[2][2], m.m[2][3],
			m.m[3][0], m.m[3][2], m.m[3][3]
		);
		result.m[1][2] = -Determinant(
			m.m[0][0], m.m[0][1], m.m[0][3],
			m.m[2][0], m.m[2][1], m.m[2][3],
			m.m[3][0], m.m[3][1], m.m[3][3]
		);
		result.m[1][3] = +Determinant(
			m.m[0][0], m.m[0][1], m.m[0][2],
			m.m[2][0], m.m[2][1], m.m[2][2],
			m.m[3][0], m.m[3][1], m.m[3][2]
		);

		// [2][]の余因子
		result.m[2][0] = +Determinant(
			m.m[0][1], m.m[0][2], m.m[0][3],
			m.m[1][1], m.m[1][2], m.m[1][3],
			m.m[3][1], m.m[3][2], m.m[3][3]
		);
		result.m[2][1] = -Determinant(
			m.m[0][0], m.m[0][2], m.m[0][3],
			m.m[1][0], m.m[1][2], m.m[1][3],
			m.m[3][0], m.m[3][2], m.m[3][3]
		);
		result.m[2][2] = +Determinant(
			m.m[0][0], m.m[0][1], m.m[0][3],
			m.m[1][0], m.m[1][1], m.m[1][3],
			m.m[3][0], m.m[3][1], m.m[3][3]
		);
		result.m[2][3] = -Determinant(
			m.m[0][0], m.m[0][1], m.m[0][2],
			m.m[1][0], m.m[1][1], m.m[1][2],
			m.m[3][0], m.m[3][1], m.m[3][2]
		);

		// [3][]の余因子
		result.m[3][0] = -Determinant(
			m.m[0][1], m.m[0][2], m.m[0][3],
			m.m[1][1], m.m[1][2], m.m[1][3],
			m.m[2][1], m.m[2][2], m.m[2][3]
		);
		result.m[3][1] = +Determinant(
			m.m[0][0], m.m[0][2], m.m[0][3],
			m.m[1][0], m.m[1][2], m.m[1][3],
			m.m[2][0], m.m[2][2], m.m[2][3]
		);
		result.m[3][2] = -Determinant(
			m.m[0][0], m.m[0][1], m.m[0][3],
			m.m[1][0], m.m[1][1], m.m[1][3],
			m.m[2][0], m.m[2][1], m.m[2][3]
		);
		result.m[3][3] = +Determinant(
			m.m[0][0], m.m[0][1], m.m[0][2],
			m.m[1][0], m.m[1][1], m.m[1][2],
			m.m[2][0], m.m[2][1], m.m[2][2]
		);

		return result;
	}

}

// ==================================================================================== helper func



namespace MathUtility {

	// Vector2 ====================================================================================

	float Dot(const Vector2 &v1, const Vector2 &v2) {
		return v1.x * v2.x + v1.y * v2.y;
	}


	float Cross(const Vector2 &v1, const Vector2 &v2) {
		return v1.x * v2.y - v1.y * v2.x;
	}


	float Length(const Vector2 &v) {
		return std::sqrt(v.x * v.x + v.y * v.y);
	}


	Vector2 Normalize(const Vector2 &v) {
		float len = Length(v);

		if (len == 0.0f) {
			return Vector2();
		}

		return Vector2(v.x / len, v.y / len);
	}

	// ==================================================================================== Vector2



	// Vector3 ====================================================================================

	float Dot(const Vector3 &v1, const Vector3 &v2) {
		return v1.x * v2.x
			+ v1.y * v2.y
			+ v1.z * v2.z;
	}


	Vector3 Cross(const Vector3 &v1, const Vector3 &v2) {
		return Vector3(
			v1.y * v2.z - v1.z * v2.y,
			v1.z * v2.x - v1.x * v2.z,
			v1.x * v2.y - v1.y * v2.x);
	}


	float Length(const Vector3 &v) {
		return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	}


	Vector3 Normalize(const Vector3 &v) {
		float len = Length(v);

		if (len == 0.0f) {
			return Vector3();
		}

		return Vector3(v.x / len, v.y / len, v.z / len);
	}

	// ==================================================================================== Vector3


	// Matrix4x4 ==================================================================================

	Matrix4x4 MakeIdentity4x4() {
		Matrix4x4 result{};

		for (int row = 0; row < 4; row++) {
			result.m[row][row] = 1.0f;
		}

		return result;
	}


	Matrix4x4 Transpose(const Matrix4x4 &m) {
		Matrix4x4 result{};

		for (int row = 0; row < 4; row++) {
			for (int col = 0; col < 4; col++) {
				result.m[row][col] = m.m[col][row];
			}
		}

		return result;
	}


	Matrix4x4 Inverse(const Matrix4x4 &m) {
		float det = Determinant(m);
		assert(det != 0.0f);

		Matrix4x4 adj = Transpose(Cofactor(m));

		Matrix4x4 result{};

		for (int row = 0; row < 4; row++) {
			for (int col = 0; col < 4; col++) {
				result.m[row][col] = adj.m[row][col] / det;
			}
		}

		return result;
	}


	Vector3 Transform(const Vector3 &v, const Matrix4x4 &m) {
		float x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
		float y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
		float z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
		float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];

		assert(w != 0.0f);

		return Vector3(x / w, y / w, z / w);
	}


	Matrix4x4 MakeTranslateMatrix(const Vector3 &translate) {
		Matrix4x4 result = MakeIdentity4x4();

		result.m[3][0] = translate.x;
		result.m[3][1] = translate.y;
		result.m[3][2] = translate.z;

		return result;
	}


	Matrix4x4 MakeScaleMatrix(const Vector3 &scale) {
		Matrix4x4 result = MakeIdentity4x4();

		result.m[0][0] = scale.x;
		result.m[1][1] = scale.y;
		result.m[2][2] = scale.z;

		return result;
	}


	Matrix4x4 MakeRotateXMatrix(float radian) {
		Matrix4x4 result = MakeIdentity4x4();

		float s = std::sin(radian);
		float c = std::cos(radian);

		result.m[1][1] = c;
		result.m[1][2] = s;
		result.m[2][1] = -s;
		result.m[2][2] = c;

		return result;
	}


	Matrix4x4 MakeRotateYMatrix(float radian) {
		Matrix4x4 result = MakeIdentity4x4();

		float s = std::sin(radian);
		float c = std::cos(radian);

		result.m[0][0] = c;
		result.m[0][2] = -s;
		result.m[2][0] = s;
		result.m[2][2] = c;

		return result;
	}


	Matrix4x4 MakeRotateZMatrix(float radian) {
		Matrix4x4 result = MakeIdentity4x4();

		float s = std::sin(radian);
		float c = std::cos(radian);

		result.m[0][0] = c;
		result.m[0][1] = s;
		result.m[1][0] = -s;
		result.m[1][1] = c;

		return result;
	}


	Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate, const Vector3 &translate) {
		Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
		Matrix4x4 rotateMatrix = MakeRotateXMatrix(rotate.x) * MakeRotateYMatrix(rotate.y) * MakeRotateZMatrix(rotate.z);
		Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

		return scaleMatrix * rotateMatrix * translateMatrix;
	}


	Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
		Matrix4x4 result{};	// 透視投影行列は[3][3] = 0.0fのためゼロ初期化

		float cot = 1.0f / std::tan(fovY / 2.0f);

		result.m[0][0] = (1.0f / aspectRatio) * cot;
		result.m[1][1] = cot;
		result.m[2][2] = farClip / (farClip - nearClip);
		result.m[2][3] = 1.0f;
		result.m[3][2] = -(nearClip * farClip) / (farClip - nearClip);

		return result;
	}


	Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
		Matrix4x4 result = MakeIdentity4x4();

		result.m[0][0] = 2.0f / (right - left);
		result.m[1][1] = 2.0f / (top - bottom);
		result.m[2][2] = 1.0f / (farClip - nearClip);
		result.m[3][0] = (left + right) / (left - right);
		result.m[3][1] = (top + bottom) / (bottom - top);
		result.m[3][2] = nearClip / (nearClip - farClip);

		return result;
	}


	Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {
		Matrix4x4 result = MakeIdentity4x4();

		result.m[0][0] = width / 2.0f;
		result.m[1][1] = -(height / 2.0f);
		result.m[2][2] = maxDepth - minDepth;
		result.m[3][0] = left + (width / 2.0f);
		result.m[3][1] = top + (height / 2.0f);
		result.m[3][2] = minDepth;

		return result;
	}

	// ================================================================================== Matrix4x4

}