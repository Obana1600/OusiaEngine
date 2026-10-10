#pragma once

#include "Math/Matrix4x4.h"
#include "Math/Vector3.h"
#include <d3d12.h>
#include <wrl.h>


namespace ou {

class WorldTransform {
public: // メンバ変数

	Vector3 translation_{};
	Vector3 scale_ = {1.0f, 1.0f, 1.0f};
	Vector3 rotation_{};

	WorldTransform *parent_ = nullptr;

	Matrix4x4 matWorld_{};


public: // メンバ関数

	WorldTransform() = default;
	~WorldTransform() = default;

	void Initialize();

	void Update();

	void TransferMatrix();

	const Microsoft::WRL::ComPtr<ID3D12Resource> &GetConstBuffer() const {
		return constBuffer_;
	}


private: // インナークラス

	struct ConstBufferData {
		Matrix4x4 matWorld;
	};


private: // メンバ変数

	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;

	ConstBufferData *constMap_ = nullptr;


private: // メンバ関数

	WorldTransform(const WorldTransform &) = delete;
	WorldTransform operator=(const WorldTransform &) = delete;
};

} // namespace ou