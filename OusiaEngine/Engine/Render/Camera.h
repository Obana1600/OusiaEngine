#pragma once

#include "Math/Matrix4x4.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Scene/Behavior.h"
#include "Scene/LayerMask.h"

#include <cstdint>

// TODO: API 名を Unity にそろえる

namespace ou {

class Camera final: public Behavior {
public: // インナークラス

	// 定数バッファ: b2
	struct PerCamera {
		static constexpr uint32_t kNoTexture = 0xFFFFFFFF;

		Matrix4x4 view;
		Matrix4x4 projection;
		Matrix4x4 viewProjection;
		Vector3 worldPosition;
		uint32_t depthTextureIndex;
		Vector4 screenSize; // (幅, 高さ, 1 / 幅, 1 / 高さ)
		uint32_t opaqueTextureIndex;
		uint32_t padding[3];
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

	void SetRenderPostProcessing(bool renderPostProcessing);
	bool GetRenderPostProcessing() const;

	void SetVolumeLayerMask(LayerMask volumeLayerMask);
	LayerMask GetVolumeLayerMask() const;


private: // メンバ変数

	float fieldOfView_ = 60.0f;
	float aspect_ = 0.0f;
	float nearClipPlane_ = 0.3f;
	float farClipPlane_ = 1000.0f;

	bool renderPostProcessing_ = true;
	LayerMask volumeLayerMask_{1u};
};

} // namespace ou
