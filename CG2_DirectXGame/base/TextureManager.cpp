#include "TextureManager.h"
#include "StringUtility.h"
#include "WinApp.h"
#include "DirectXCommon.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include <cassert>
#include <cstdint>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace Microsoft::WRL;


namespace {

	ComPtr<ID3D12Resource> CreateBufferResource(const ComPtr<ID3D12Device> &device, const size_t sizeInBytes) {
		// ヒープの設定
		D3D12_HEAP_PROPERTIES uploadHeapProperties{};
		uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // UploadHeapを使う
		// リソースの設定
		D3D12_RESOURCE_DESC resourceDesc{};
		// バッファリソース
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = sizeInBytes; // リソースのサイズ
		// バッファの場合はこれらは1にする決まり
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc.Count = 1;
		// バッファの場合はこれにする決まり
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		// 実際にリソースを作る
		ComPtr<ID3D12Resource> resource{};
		HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
			&resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
		assert(SUCCEEDED(hr));

		return resource;
	}


	DirectX::ScratchImage LoadTexture(const std::string &filePath) {
		// テクスチャファイルを読んでプログラムで扱えるようにする
		DirectX::ScratchImage image{};
		std::wstring filePathW = ConvertStringMultiByteToWide(filePath);
		HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
		assert(SUCCEEDED(hr));

		// ミップマップの作成
		DirectX::ScratchImage mipImages{};
		hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
		assert(SUCCEEDED(hr));

		// ミップマップ付きのデータを返す
		return mipImages;
	}


	ComPtr<ID3D12Resource> CreateTextureResource(const ComPtr<ID3D12Device> &device, const DirectX::TexMetadata &metadata) {
		// metadataを基にResourceの設定
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Width = UINT(metadata.width); // Textureの幅
		resourceDesc.Height = UINT(metadata.height); // Textureの高さ
		resourceDesc.MipLevels = UINT16(metadata.mipLevels); // mipmapの数
		resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize); // 奥行き or 配列Textureの配列数
		resourceDesc.Format = metadata.format; // TextureのFormat
		resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数。普段使っているのは2次元

		// 利用するHeapの設定
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // 細かい設定を行う

		// Resourceの生成
		ComPtr<ID3D12Resource> resource{};
		HRESULT hr = device->CreateCommittedResource(
			&heapProperties, // Heapの設定
			D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。特になし
			&resourceDesc, // Resourceの設定
			D3D12_RESOURCE_STATE_COPY_DEST, // データ転送される設定
			nullptr, // Clear最適値。使わないのでnullptr
			IID_PPV_ARGS(&resource) // 作成するResourceポインタへのポインタ
		);
		assert(SUCCEEDED(hr));

		return resource;
	}


	[[nodiscard]]
	ComPtr<ID3D12Resource> UploadTextureData(
		const ComPtr<ID3D12Resource> &texture, const DirectX::ScratchImage &mipImages,
		const ComPtr<ID3D12Device> &device, ID3D12GraphicsCommandList *commandList
	) {
		std::vector<D3D12_SUBRESOURCE_DATA> subresources{};
		DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
		uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
		ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(device, intermediateSize);
		UpdateSubresources(commandList, texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());
		// Textureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_RAEDへResourceStateを変更する
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = texture.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		commandList->ResourceBarrier(1, &barrier);
		return intermediateResource;
	}


	ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(const ComPtr<ID3D12Device> &device, int32_t width, int32_t height) {
		// 生成するResourceの設定
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Width = width; // Textureの幅
		resourceDesc.Height = height; // Textureの高さ
		resourceDesc.MipLevels = 1; // mipmapの数
		resourceDesc.DepthOrArraySize = 1; // 奥行き or 配列Textureの配列数
		resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // DepthStencilとして利用可能なフォーマット
		resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2次元
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

		// 利用するHeapの設定
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

		// 深度値のクリア設定
		D3D12_CLEAR_VALUE depthClearValue{};
		depthClearValue.DepthStencil.Depth = 1.0f; // 1.0f（最大値）でクリア
		depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット。Resourceと合わせる

		// Resourceの生成
		ComPtr<ID3D12Resource> resource{};
		HRESULT hr = device->CreateCommittedResource(
			&heapProperties, // Heapの設定
			D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。特になし
			&resourceDesc, // Resourceの設定
			D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度地を書き込む状態にしておく
			&depthClearValue, // Clear最適値
			IID_PPV_ARGS(&resource) // 作成するResourceポインタへのポインタ
		);
		assert(SUCCEEDED(hr));

		return resource;
	}


	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap> &descriptorHeap, uint32_t descriptorSize, uint32_t index) {
		D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		handleCPU.ptr += (descriptorSize * index);
		return handleCPU;
	}


	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap> &descriptorHeap, uint32_t descriptorSize, uint32_t index) {
		D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
		handleGPU.ptr += (descriptorSize * index);
		return handleGPU;
	}
}



TextureManager *TextureManager::GetInstance() {
	static TextureManager instance{};
	return &instance;
}


uint32_t TextureManager::Load(const std::string &fileName) {
	return TextureManager::GetInstance()->LoadInternal(fileName);
}


void TextureManager::Initialize(const std::string &directoryPath) {
	m_device = DirectXCommon::GetInstance()->GetDevice();
	m_commandListForTransfer = DirectXCommon::GetInstance()->GetCommandListForTransfer();
	m_directoryPath = directoryPath;
	m_useTable.resize(kNumDescriptors, false);

	// SRVヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC descriptorDesc{};
	descriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorDesc.NumDescriptors = static_cast<UINT>(kNumDescriptors);
	descriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = m_device->CreateDescriptorHeap(&descriptorDesc, IID_PPV_ARGS(&m_srvHeap));
	assert(SUCCEEDED(hr));

	m_sDescriptorHandleIncrementSize = m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}


void TextureManager::SetGraphicsRootDescriptorTable(ID3D12GraphicsCommandList *commandList, UINT rootParamIndex, uint32_t textureHandle) {
	commandList->SetDescriptorHeaps(1, m_srvHeap.GetAddressOf());
	commandList->SetGraphicsRootDescriptorTable(rootParamIndex, m_textures[textureHandle].gpuDescHandleSRV);
}


uint32_t TextureManager::LoadInternal(const std::string &fileName) {
	// 空きスロットを探す
	uint32_t slot = 0;
	for (size_t i = 0; i < m_useTable.size(); i++) {
		if (!m_useTable[i]) {
			slot = static_cast<uint32_t>(i);
			break;
		}
	}
	m_useTable[slot] = true;

	// テクスチャ読み込み
	DirectX::ScratchImage mipImages = LoadTexture(fileName);
	const DirectX::TexMetadata &metadata = mipImages.GetMetadata();

	// リソース作成
	m_textures[slot].resource = CreateTextureResource(m_device, metadata);
	m_intermediateResources.push_back(UploadTextureData(m_textures[slot].resource, mipImages, m_device, m_commandListForTransfer));

	// SRV作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	m_textures[slot].name = fileName;
	m_textures[slot].cpuDescHandleSRV = GetCPUDescriptorHandle(m_srvHeap, m_sDescriptorHandleIncrementSize, slot);
	m_textures[slot].gpuDescHandleSRV = GetGPUDescriptorHandle(m_srvHeap, m_sDescriptorHandleIncrementSize, slot);
	m_device->CreateShaderResourceView(m_textures[slot].resource.Get(), &srvDesc, m_textures[slot].cpuDescHandleSRV);

	return slot;
}
