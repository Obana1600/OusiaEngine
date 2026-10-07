#pragma once

#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>


namespace ou {

class GpuResourceOld {
public: // メンバ関数

	// コンストラクタ / デストラクタ
	GpuResourceOld() = default;
	GpuResourceOld(ID3D12Resource *resource, D3D12_RESOURCE_STATES currentState);
	~GpuResourceOld();

	// std::vector に入れるためにムーブは許可する
	GpuResourceOld(GpuResourceOld &&) = default;
	GpuResourceOld &operator=(GpuResourceOld &&) = default;

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

	GpuResourceOld(const GpuResourceOld &) = delete;
	GpuResourceOld &operator=(const GpuResourceOld &) = delete;
};

} // namespace ou
