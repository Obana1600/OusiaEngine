#pragma once

#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>


class GpuResource {
public: // メンバ関数

	// コンストラクタ / デストラクタ
	GpuResource() = default;
	GpuResource(ID3D12Resource *resource, D3D12_RESOURCE_STATES currentState);
	~GpuResource();

	// std::vector に入れるためにムーブは許可する
	GpuResource(GpuResource &&) = default;
	GpuResource &operator=(GpuResource &&) = default;

	virtual void Destroy();

	ID3D12Resource *Resource() const;
	D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress() const;
	uint32_t VersionId() const;


protected: // メンバ変数

	Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
	D3D12_RESOURCE_STATES usageState_ = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_STATES transitioningState_ = (D3D12_RESOURCE_STATES)-1;
	D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress_{};

	uint32_t versionId_ = 0;


private: // コピー禁止

	GpuResource(const GpuResource &) = delete;
	GpuResource &operator=(const GpuResource &) = delete;
};
