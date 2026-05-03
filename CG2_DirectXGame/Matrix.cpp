#include "Matrix.h"
#include <cmath>
#include <cassert>


namespace MathUtility {

	// Matrix2x2 ==================================================================================

	Matrix2x2 Add(const Matrix2x2 &m1, const Matrix2x2 &m2) {
		Matrix2x2 result{};

		result.m[0][0] = m1.m[0][0] + m2.m[0][0];
		result.m[1][0] = m1.m[1][0] + m2.m[1][0];
		result.m[0][1] = m1.m[0][1] + m2.m[0][1];
		result.m[1][1] = m1.m[1][1] + m2.m[1][1];

		return result;
	}


	Matrix2x2 Subtract(const Matrix2x2 &m1, const Matrix2x2 &m2) {
		Matrix2x2 result{};

		result.m[0][0] = m1.m[0][0] - m2.m[0][0];
		result.m[1][0] = m1.m[1][0] - m2.m[1][0];
		result.m[0][1] = m1.m[0][1] - m2.m[0][1];
		result.m[1][1] = m1.m[1][1] - m2.m[1][1];

		return result;
	}


	Matrix2x2 Multiply(const Matrix2x2 &m1, const Matrix2x2 &m2) {
		Matrix2x2 result{};

		result.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0];
		result.m[1][0] = m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0];
		result.m[0][1] = m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1];
		result.m[1][1] = m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1];

		return result;
	}


	Vector2 Multiply(const Vector2 &v, const Matrix2x2 &m) {
		Vector2 result{};

		result.x = v.x * m.m[0][0] + v.y * m.m[1][0];
		result.y = v.x * m.m[0][1] + v.y * m.m[1][1];

		return result;
	}


	Matrix2x2 MakeRotateMatrix2x2(float theta) {
		Matrix2x2 result{};

		float s = std::sin(theta);
		float c = std::cos(theta);

		result.m[0][0] = c;
		result.m[0][1] = s;
		result.m[1][0] = -s;
		result.m[1][1] = c;

		return result;
	}


	float Determinant(const Matrix2x2 &m) {
		return m.m[0][0] * m.m[1][1] - m.m[0][1] * m.m[1][0];
	}


	Matrix3x3 MakeTranslateMatrix(const Vector2 &transform) {
		Matrix3x3 result{};

		result.m[0][0] = 1.0f;
		result.m[2][0] = transform.x;
		result.m[1][1] = 1.0f;
		result.m[2][1] = transform.y;
		result.m[2][2] = 1.0f;

		return result;
	}


	Vector2 Transform(const Vector2 &v, const Matrix3x3 &m) {
		Vector2 result{};
		result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + 1.0f * m.m[2][0];
		result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + 1.0f * m.m[2][1];

		float w = v.x * m.m[0][2] + v.y * m.m[1][2] + 1.0f * m.m[2][2];
		assert(w != 0.0f);

		result.x /= w;
		result.y /= w;

		return result;
	}


	Matrix2x2 Inverse(const Matrix2x2 &m) {
		Matrix2x2 result{};

		float det = Determinant(m);
		assert(det != 0.0f);

		result.m[0][0] = m.m[1][1] / det;
		result.m[0][1] = -m.m[0][1] / det;
		result.m[1][0] = -m.m[1][0] / det;
		result.m[1][1] = m.m[0][0] / det;

		return result;
	}


	Matrix2x2 Transpose(const Matrix2x2 &m) {
		Matrix2x2 result{};

		result.m[0][0] = m.m[0][0];
		result.m[0][1] = m.m[1][0];
		result.m[1][0] = m.m[0][1];
		result.m[1][1] = m.m[1][1];

		return result;
	}

	// ================================================================================== Matrix2x2



	// Matrix3x3 ==================================================================================

	Matrix3x3 Multiply(const Matrix3x3 &m1, const Matrix3x3 &m2) {
		Matrix3x3 result{};

		result.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] + m1.m[0][2] * m2.m[2][0];
		result.m[0][1] = m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] + m1.m[0][2] * m2.m[2][1];
		result.m[0][2] = m1.m[0][0] * m2.m[0][2] + m1.m[0][1] * m2.m[1][2] + m1.m[0][2] * m2.m[2][2];
		result.m[1][0] = m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0] + m1.m[1][2] * m2.m[2][0];
		result.m[1][1] = m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1] + m1.m[1][2] * m2.m[2][1];
		result.m[1][2] = m1.m[1][0] * m2.m[0][2] + m1.m[1][1] * m2.m[1][2] + m1.m[1][2] * m2.m[2][2];
		result.m[2][0] = m1.m[2][0] * m2.m[0][0] + m1.m[2][1] * m2.m[1][0] + m1.m[2][2] * m2.m[2][0];
		result.m[2][1] = m1.m[2][0] * m2.m[0][1] + m1.m[2][1] * m2.m[1][1] + m1.m[2][2] * m2.m[2][1];
		result.m[2][2] = m1.m[2][0] * m2.m[0][2] + m1.m[2][1] * m2.m[1][2] + m1.m[2][2] * m2.m[2][2];

		return result;
	}


	Matrix3x3 MakeRotateMatrix3x3(float theta) {
		Matrix3x3 result{};

		float s = std::sin(theta);
		float c = std::cos(theta);

		result.m[0][0] = c;
		result.m[0][1] = -s;
		result.m[1][0] = s;
		result.m[1][1] = c;
		result.m[2][2] = 1.0f;

		return result;
	}


	Matrix3x3 MakeScaleMatrix(const Vector2 &scale) {
		Matrix3x3 result{};

		result.m[0][0] = scale.x;
		result.m[1][1] = scale.y;
		result.m[2][2] = 1.0f;

		return result;
	}


	Matrix3x3 MakeAffineMatrix(const Vector2 &translate, const Vector2 &scale, float rotate) {
		Matrix3x3 scaleMatrix = MakeScaleMatrix(scale);
		Matrix3x3 rotateMatrix = MakeRotateMatrix3x3(rotate);
		Matrix3x3 translateMatrix = MakeTranslateMatrix(translate);

		Matrix3x3 tempMatrix = Multiply(scaleMatrix, rotateMatrix);
		Matrix3x3 worldMatrix = Multiply(tempMatrix, translateMatrix);

		return worldMatrix;
	}


	float Determinant(const Matrix3x3 &m) {
		return Determinant(
			m.m[0][0], m.m[0][1], m.m[0][2],
			m.m[1][0], m.m[1][1], m.m[1][2],
			m.m[2][0], m.m[2][1], m.m[2][2]
		);
	}


	float Determinant(float a00, float a01, float a02, float a10, float a11, float a12, float a20, float a21, float a22) {
		return a00 * (a11 * a22 - a12 * a21)
			- a10 * (a01 * a22 - a02 * a21)
			+ a20 * (a01 * a12 - a02 * a11);
	}


	Matrix3x3 Inverse(const Matrix3x3 &m) {
		float det = Determinant(m);
		assert(det != 0.0f);

		Matrix3x3 result{};

		result.m[0][0] = (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1]) / det;
		result.m[0][1] = -(m.m[0][1] * m.m[2][2] - m.m[0][2] * m.m[2][1]) / det;
		result.m[0][2] = (m.m[0][1] * m.m[1][2] - m.m[0][2] * m.m[1][1]) / det;
		result.m[1][0] = -(m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0]) / det;
		result.m[1][1] = (m.m[0][0] * m.m[2][2] - m.m[0][2] * m.m[2][0]) / det;
		result.m[1][2] = -(m.m[0][0] * m.m[1][2] - m.m[0][2] * m.m[1][0]) / det;
		result.m[2][0] = (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0]) / det;
		result.m[2][1] = -(m.m[0][0] * m.m[2][1] - m.m[0][1] * m.m[2][0]) / det;
		result.m[2][2] = (m.m[0][0] * m.m[1][1] - m.m[0][1] * m.m[1][0]) / det;

		return result;
	}


	Matrix3x3 Transpose(const Matrix3x3 &m) {
		Matrix3x3 result{};

		result.m[0][0] = m.m[0][0];
		result.m[0][1] = m.m[1][0];
		result.m[0][2] = m.m[2][0];
		result.m[1][0] = m.m[0][1];
		result.m[1][1] = m.m[1][1];
		result.m[1][2] = m.m[2][1];
		result.m[2][0] = m.m[0][2];
		result.m[2][1] = m.m[1][2];
		result.m[2][2] = m.m[2][2];

		return result;
	}


	Matrix3x3 MakeCameraWorldMatrix(float positionWorldX, float positionWorldY) {
		Matrix3x3 result{};

		result.m[0][0] = 1.0f;
		result.m[1][1] = 1.0f;
		result.m[2][0] = positionWorldX;
		result.m[2][1] = positionWorldY;
		result.m[2][2] = 1.0f;

		return result;
	}


	Matrix3x3 MakeOrthographicMatrix(float left, float top, float right, float bottom) {
		Matrix3x3 result{};

		result.m[0][0] = 2.0f / (right - left);
		result.m[1][1] = 2.0f / (top - bottom);
		result.m[2][0] = (left + right) / (left - right);
		result.m[2][1] = (top + bottom) / (bottom - top);
		result.m[2][2] = 1.0f;

		return result;
	}


	Matrix3x3 MakeViewportMatrix(float left, float top, float width, float height) {
		Matrix3x3 result{};

		result.m[0][0] = width / 2.0f;
		result.m[1][1] = -height / 2.0f;
		result.m[2][0] = left + width / 2.0f;
		result.m[2][1] = top + height / 2.0f;
		result.m[2][2] = 1.0f;

		return result;
	}

	// ================================================================================== Matrix3x3



	// Matrix4x4 ==================================================================================

	Matrix4x4 Add(const Matrix4x4 &m1, const Matrix4x4 &m2) {
		Matrix4x4 result{};

		for (int row = 0; row < 4; row++) {
			for (int column = 0; column < 4; column++) {
				result.m[row][column] = m1.m[row][column] + m2.m[row][column];
			}
		}

		return result;
	}


	Matrix4x4 Subtract(const Matrix4x4 &m1, const Matrix4x4 &m2) {
		Matrix4x4 result{};

		for (int row = 0; row < 4; row++) {
			for (int column = 0; column < 4; column++) {
				result.m[row][column] = m1.m[row][column] - m2.m[row][column];
			}
		}

		return result;
	}


	Matrix4x4 Multiply(const Matrix4x4 &m1, const Matrix4x4 &m2) {
		Matrix4x4 result{};

		for (int row = 0; row < 4; row++) {
			for (int column = 0; column < 4; column++) {
				for (int k = 0; k < 4; k++) {
					result.m[row][column] += m1.m[row][k] * m2.m[k][column];
				}
			}
		}

		return result;
	}


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


	Matrix4x4 Inverse(const Matrix4x4 &m) {
		float det = Determinant(m);
		assert(det != 0.0f);

		Matrix4x4 adj = Transpose(Cofactor(m));

		Matrix4x4 result{};

		for (int row = 0; row < 4; row++) {
			for (int column = 0; column < 4; column++) {
				result.m[row][column] = adj.m[row][column] / det;
			}
		}

		return result;
	}


	Matrix4x4 Transpose(const Matrix4x4 &m) {
		Matrix4x4 result{};

		for (int row = 0; row < 4; row++) {
			for (int column = 0; column < 4; column++) {
				result.m[row][column] = m.m[column][row];
			}
		}

		return result;
	}


	Matrix4x4 MakeIdentity4x4() {
		Matrix4x4 result{};

		for (int i = 0; i < 4; i++) {
			result.m[i][i] = 1.0f;
		}

		return result;
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


	Vector3 Transform(const Vector3 &v, const Matrix4x4 &m) {
		float x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
		float y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
		float z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
		float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];

		assert(w != 0.0f);

		return { x / w, y / w, z / w };
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

	Matrix4x4 operator*(const Matrix4x4 &m1, const Matrix4x4 &m2) {
		return Multiply(m1, m2);
	}

	// ================================================================================== Matrix4x4

} // MathUtility