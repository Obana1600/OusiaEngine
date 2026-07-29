#include "TextureManager.h"

#include "CreateBufferResource.h"
#include "DirectXCommon.h"
#include "StringUtility.h"
#include "WinApp.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include <cassert>
#include <cstdint>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace Microsoft::WRL;


namespace {

	DirectX::ScratchImage LoadTexture(const std::string &filePath) {
		// テクスチャファイルを読んでプログラムで扱えるようにする
		DirectX::ScratchImage image{};
		std::wstring filePathW = ConvertStringMultiByteToWide(filePath);
		HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
		assert(SUCCEEDED(hr));

		// 1x1はミップマップチェーンを作れない
		if (image.GetMetadata().width <= 1 && image.GetMetadata().height <= 1) {
			return image;
		}

		// ミップマップの作成
		DirectX::ScratchImage mipImages{};
		hr = DirectX::GenerateMipMaps(
			image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages
		);
		assert(SUCCEEDED(hr));

		// ミップマップ付きのデータを返す
		return mipImages;
	}


	ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device *device, const DirectX::TexMetadata &metadata) {
		// metadataを基にResourceの設定
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Width = UINT(metadata.width);							   // Textureの幅
		resourceDesc.Height = UINT(metadata.height);						   // Textureの高さ
		resourceDesc.MipLevels = UINT16(metadata.mipLevels);				   // mipmapの数
		resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);			   // 奥行き or 配列Textureの配列数
		resourceDesc.Format = metadata.format;								   // TextureのFormat
		resourceDesc.SampleDesc.Count = 1;									   // サンプリングカウント。1固定
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数。普段使っているのは2次元

		// 利用するHeapの設定
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // 細かい設定を行う

		// Resourceの生成
		ComPtr<ID3D12Resource> resource{};
		HRESULT hr = device->CreateCommittedResource(
			&heapProperties,				// Heapの設定
			D3D12_HEAP_FLAG_NONE,			// Heapの特殊な設定。特になし
			&resourceDesc,					// Resourceの設定
			D3D12_RESOURCE_STATE_COPY_DEST, // データ転送される設定
			nullptr,						// Clear最適値。使わないのでnullptr
			IID_PPV_ARGS(&resource)			// 作成するResourceポインタへのポインタ
		);
		assert(SUCCEEDED(hr));

		return resource;
	}


	[[nodiscard]]
	ComPtr<ID3D12Resource> UploadTextureData(
		ID3D12Resource *texture,
		const DirectX::ScratchImage &mipImages,
		ID3D12Device *device,
		ID3D12GraphicsCommandList *commandList
	) {
		std::vector<D3D12_SUBRESOURCE_DATA> subresources{};
		DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
		uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
		ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(device, intermediateSize);
		UpdateSubresources(commandList, texture, intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());
		// Textureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_RAEDへResourceStateを変更する
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = texture;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		commandList->ResourceBarrier(1, &barrier);
		return intermediateResource;
	}


	ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device *device, int32_t width, int32_t height) {
		// 生成するResourceの設定
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Width = width;									  // Textureの幅
		resourceDesc.Height = height;								  // Textureの高さ
		resourceDesc.MipLevels = 1;									  // mipmapの数
		resourceDesc.DepthOrArraySize = 1;							  // 奥行き or 配列Textureの配列数
		resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;		  // DepthStencilとして利用可能なフォーマット
		resourceDesc.SampleDesc.Count = 1;							  // サンプリングカウント。1固定
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // 2次元
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

		// 利用するHeapの設定
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

		// 深度値のクリア設定
		D3D12_CLEAR_VALUE depthClearValue{};
		depthClearValue.DepthStencil.Depth = 1.0f;				// 1.0f（最大値）でクリア
		depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット。Resourceと合わせる

		// Resourceの生成
		ComPtr<ID3D12Resource> resource{};
		HRESULT hr = device->CreateCommittedResource(
			&heapProperties,				  // Heapの設定
			D3D12_HEAP_FLAG_NONE,			  // Heapの特殊な設定。特になし
			&resourceDesc,					  // Resourceの設定
			D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度地を書き込む状態にしておく
			&depthClearValue,				  // Clear最適値
			IID_PPV_ARGS(&resource)			  // 作成するResourceポインタへのポインタ
		);
		assert(SUCCEEDED(hr));

		return resource;
	}


	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap *descriptorHeap, uint32_t descriptorSize, uint32_t index) {
		D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		handleCPU.ptr += (descriptorSize * index);
		return handleCPU;
	}


	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap *descriptorHeap, uint32_t descriptorSize, uint32_t index) {
		D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
		handleGPU.ptr += (descriptorSize * index);
		return handleGPU;
	}

} // namespace



TextureManager *TextureManager::GetInstance() {
	static TextureManager instance{};
	return &instance;
}


uint32_t TextureManager::Load(const std::string &fileName) {
	return TextureManager::GetInstance()->LoadInternal(fileName);
}


void TextureManager::Initialize(const std::string &directoryPath) {
	device_ = DirectXCommon::GetInstance()->GetDevice();
	commandListForTransfer_ = DirectXCommon::GetInstance()->GetCommandListForTransfer();
	directoryPath_ = directoryPath;
	useTable_.resize(kNumDescriptors, false);

	// SRVヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC descriptorDesc{};
	descriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorDesc.NumDescriptors = static_cast<UINT>(kNumDescriptors);
	descriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = device_->CreateDescriptorHeap(&descriptorDesc, IID_PPV_ARGS(&srvHeap_));
	assert(SUCCEEDED(hr));

	descriptorHandleIncrementSize_ = device_->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);
}


void TextureManager::SetGraphicsRootDescriptorTable(ID3D12GraphicsCommandList *commandList, UINT rootParamIndex, uint32_t textureHandle) {
	commandList->SetDescriptorHeaps(1, srvHeap_.GetAddressOf());
	commandList->SetGraphicsRootDescriptorTable(rootParamIndex, textures_[textureHandle].gpuDescHandleSRV);
}


uint32_t TextureManager::LoadInternal(const std::string &fileName) {
	// 読み込み済みならそのハンドルを返す
	for (size_t i = 0; i < useTable_.size(); i++) {
		if (useTable_[i] && textures_[i].name == fileName) {
			return static_cast<uint32_t>(i);
		}
	}

	// 空きスロットを探す
	uint32_t slot = UINT32_MAX;
	for (size_t i = 0; i < useTable_.size(); i++) {
		if (!useTable_[i]) {
			slot = static_cast<uint32_t>(i);
			break;
		}
	}
	assert(slot != UINT32_MAX); // ディスクリプタに空きがない
	useTable_[slot] = true;

	// テクスチャ読み込み
	DirectX::ScratchImage mipImages = LoadTexture(directoryPath_ + fileName);
	const DirectX::TexMetadata &metadata = mipImages.GetMetadata();

	// リソース作成
	textures_[slot].resource = CreateTextureResource(device_, metadata);
	intermediateResources_.push_back(UploadTextureData(textures_[slot].resource.Get(), mipImages, device_, commandListForTransfer_));

	// SRV作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	textures_[slot].name = fileName;
	textures_[slot].cpuDescHandleSRV = GetCPUDescriptorHandle(srvHeap_.Get(), descriptorHandleIncrementSize_, slot);
	textures_[slot].gpuDescHandleSRV = GetGPUDescriptorHandle(srvHeap_.Get(), descriptorHandleIncrementSize_, slot);
	device_->CreateShaderResourceView(textures_[slot].resource.Get(), &srvDesc, textures_[slot].cpuDescHandleSRV);

	DirectXCommon::GetInstance()->ExecuteTransferCommands();

	return slot;
}