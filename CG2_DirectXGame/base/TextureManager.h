#pragma once

#include "externals/DirectXTex/d3dx12.h"
#include <string>
#include <wrl.h>
#include <d3d12.h>
#include <vector>
#include <array>


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


private: // メンバ変数

	ID3D12Device *m_device = nullptr;

	UINT m_sDescriptorHandleIncrementSize = 0u;

	std::string m_directoryPath{};

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap{};

	std::array<Texture, kNumDescriptors> m_textures{};
	std::vector<bool> m_useTable{};

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_intermediateResources{};

	ID3D12GraphicsCommandList *m_commandListForTransfer = nullptr;


private: // メンバ関数

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(const TextureManager &) = delete;
	const TextureManager &operator=(const TextureManager &) = delete;

	uint32_t LoadInternal(const std::string &filePath);

}; 