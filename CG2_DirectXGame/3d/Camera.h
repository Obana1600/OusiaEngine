#pragma once

#include "math/Matrix4x4.h"
#include "math/Vector3.h"
#include <d3d12.h>
#include <numbers>
#include <wrl.h>


class Camera {
public: // メンバ変数

	Vector3 translation_ = {0.0f, 0.0f, -50.0f};
	Vector3 rotation_{};

	float fovAngleY_ = 45.0f * std::numbers::pi_v<float> / 180.0f;
	float aspectRatio_ = 16.0f / 9.0f;
	float nearZ_ = 0.1f;
	float farZ_ = 1000.0f;

	Matrix4x4 matView_{};
	Matrix4x4 matProjection_{};


public: // メンバ関数

	Camera() = default;
	~Camera() = default;

	void Initialize();

	void Update();

	void TransferMatrix();

	const Microsoft::WRL::ComPtr<ID3D12Resource> GetConstBuffer() const {
		return constBuffer_;
	}

private: // インナークラス

	struct ConstBufferData {
		Matrix4x4 view;
		Matrix4x4 projection;
		Vector3 translation;
	};


private: // メンバ変数

	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;

	ConstBufferData *constMap_ = nullptr;


private: // メンバ関数

	Camera(const Camera &) = delete;
	Camera operator=(const Camera &) = delete;
};