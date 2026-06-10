#pragma once

#include "math/Matrix4x4.h"
#include "math/Vector3.h"
#include <math/MathUtility.h>


class DebugCamera {
public: // public関数

	void Initialize();

	void Update();


	// getter ------------------------------------------------------------

	const Matrix4x4 &GetViewProjectionMatrix() const {
		return m_viewProjectionMatrix;
	}

	const Matrix4x4 &GetViewportMatrix() const {
		return m_viewportMatrix;
	}

	// -------------------------------------------------------------------


private: // private変数

	Vector3 m_pivot = { 0.0f, 0.0f, 0.0f };
	Matrix4x4 m_matRot = MathUtility::MakeIdentity4x4();
	float m_distance = 5.0f;

	Matrix4x4 m_viewProjectionMatrix{};
	Matrix4x4 m_viewportMatrix{};


private:

	void UpdateTransform();

	void UpdateMatrix();

};