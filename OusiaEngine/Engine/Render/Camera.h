#pragma once

#include "Math/Matrix4x4.h"
#include "Math/Vector3.h"
#include "Scene/Behavior.h"

// TODO: API 名を Unity にそろえる

namespace ou {

class Camera final: public Behavior {
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
	void Serialize(Serializer &serializer) override;
	void OnDrawGizmos() override;
	void OnDrawGizmosSelected() override;

	Matrix4x4 GetWorldToCameraMatrix() const;
	Matrix4x4 GetProjectionMatrix() const;

	PerCamera MakePerCamera() const;

	void SetFieldOfView(float fieldOfView);
	float GetFieldOfView() const;

	void SetAspect(float aspect);
	float GetAspect() const;
	void ResetAspect();

	void SetNearClipPlane(float nearClipPlane);
	float GetNearClipPlane() const;

	void SetFarClipPlane(float farClipPlane);
	float GetFarClipPlane() const;


private: // メンバ変数

	float fieldOfView_ = 60.0f;
	float aspect_ = 0.0f;
	float nearClipPlane_ = 0.3f;
	float farClipPlane_ = 1000.0f;
};

} // namespace ou
