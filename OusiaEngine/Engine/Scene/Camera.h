#pragma once

#include "Math/Mathf.h"
#include "Math/Matrix4x4.h"
#include "Math/Vector3.h"
#include "Scene/Component.h"

// TODO: API 名を Unity にそろえる

namespace ou {

class Camera final: public Component {
public: // インナークラス

	// 定数バッファ: b2
	struct PerCamera {
		Matrix4x4 view;
		Matrix4x4 projection;
		Matrix4x4 viewProjection;
		Vector3 worldPosition;
		float padding;
	};


public: // メンバ関数

	void Awake() override;
	void OnDestroy() override;

	Matrix4x4 GetViewMatrix() const;
	Matrix4x4 GetProjectionMatrix() const;

	PerCamera MakePerCamera() const;

	void SetFovY(float fovY);
	float GetFovY() const;

	void SetAspect(float aspect);
	float GetAspect() const;

	void SetClipPlanes(float nearZ, float farZ);
	float GetNearZ() const;
	float GetFarZ() const;


private: // メンバ変数

	float fovY_ = 60.0f * Mathf::Deg2Rad;
	float aspect_ = 16.0f / 9.0f;
	float nearZ_ = 0.1f;
	float farZ_ = 1000.0f;
};

} // namespace ou
