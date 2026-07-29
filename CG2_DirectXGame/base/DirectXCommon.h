#pragma once

#include "WinApp.h"
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <wrl.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")


class DirectXCommon {
public: // 静的メンバ関数

	static DirectXCommon *GetInstance();


public: // メンバ関数

	void Initialize(
		int32_t backBufferWidth = WinApp::kWindowWidth,
		int32_t backBufferHeight = WinApp::kWindowHeight,
		bool enableDebugLayer = false
	);

	void Finalize();

	void ExecuteTransferCommands();

	void PreDraw();

	void PostDraw();

	ID3D12Device *GetDevice() const {
		return device_.Get();
	}

	ID3D12GraphicsCommandList *GetCommandListForTransfer() const {
		return commandListForTransfer_.Get();
	}

	ID3D12GraphicsCommandList *GetCommandListForRender() const {
		return commandListForRender_.Get();
	}

	size_t GetBackBufferCount() const {
		return backBuffers_.size();
	}


private: // メンバ変数

	WinApp *winApp_ = nullptr;

	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandListForTransfer_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandListForRender_;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocatorForTransfer_;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocatorForRender_;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> backBuffers_;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resourceForTransfer_;
	HANDLE fenceEvent_;
	UINT64 fenceVal_ = 0;
	int32_t backBufferWidth_ = 0;
	int32_t backBufferHeight_ = 0;

	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissorRect_{};

private: // メンバ関数

	DirectXCommon() = default;
	~DirectXCommon() = default;
	DirectXCommon(const DirectXCommon &) = delete;
	const DirectXCommon &operator=(const DirectXCommon &) = delete;

	void InitializeDXGIDevice(bool enableDebugLayer = false);
	void InitializeCommand();

	void CreateSwapChain();
	void CreateFinalRenderTargets();
	void CreateDepthBuffer();
	void CreateFence();
	void CreateViewport();
	void CreateScissor();

};