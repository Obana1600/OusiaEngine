#pragma once

#include "externals/DirectXTex/d3dx12.h"
#include <array>
#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>


namespace ou {

class TextureManager {
public: // インナークラス

	struct Texture {
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescHandleSRV;
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescHandleSRV;
		std::string name;
	};


public: // 静的メンバ変数

	static const size_t kNumDescriptors = 1024;


public: // 静的メンバ関数

	static TextureManager *GetInstance();

	static uint32_t Load(const std::string &fileName);


public: // メンバ関数

	void Initialize(const std::string &directoryPath = "resources/");

	void SetGraphicsRootDescriptorTable(ID3D12GraphicsCommandList *commandList, UINT rootParamIndex, uint32_t textureHandle);

	D3D12_RESOURCE_DESC GetResourceDesc(uint32_t textureHandle) const {
		return textures_[textureHandle].resource->GetDesc();
	}


private: // メンバ変数


	UINT descriptorHandleIncrementSize_ = 0u;

	std::string directoryPath_{};

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_{};

	std::array<Texture, kNumDescriptors> textures_{};
	std::vector<bool> useTable_{};

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> intermediateResources_{};

	ID3D12Device *device_ = nullptr;
	ID3D12GraphicsCommandList *commandListForTransfer_ = nullptr;


private: // メンバ関数

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(const TextureManager &) = delete;
	const TextureManager &operator=(const TextureManager &) = delete;

	uint32_t LoadInternal(const std::string &filePath);
};

} // namespace ou
