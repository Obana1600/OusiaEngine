#pragma once

#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include <d3d12.h>
#include <wrl.h>


namespace ou {

class DirectionLight {
public: // メンバ変数

	Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f};

	Vector3 direction_ = {1.0f, 0.0f, 0.0f};

	float intensity_ = 1.0f;


public: // メンバ関数

	DirectionLight() = default;

	void Initialize();

	void TransferData();

	const Microsoft::WRL::ComPtr<ID3D12Resource> &GetConstBuffer() const {
		return constBuffer_;
	}


private: // インナークラス

	struct ConstBufferData {
		Vector4 color;	   // ライトの色
		Vector3 direction; // ライトの向き
		float intensity;   // 輝度
	};


private: // メンバ変数

	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;

	ConstBufferData *constMap_ = nullptr;


private: // メンバ関数

	DirectionLight(const DirectionLight &) = delete;
	DirectionLight operator=(const DirectionLight &) = delete;
};

} // namespace ou