#pragma once

#include "WinApp.h"
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <dxgi1_6.h>

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

	void PreDraw();

	void PostDraw();

	ID3D12Device *GetDevice() const {
		return m_device.Get();
	}

	ID3D12GraphicsCommandList *GetCommandListForTransfer() const {
		return m_commandListForTransfer.Get();
	}

	ID3D12GraphicsCommandList *GetCommandListForRender() const {
		return m_commandListForRender.Get();
	}

	void ExecuteTransferCommands();

	size_t GetBackBufferCount() const {
		return m_backBuffers.size();
	}


private: // メンバ変数

	WinApp *m_winApp = nullptr;

	Microsoft::WRL::ComPtr<IDXGIFactory7> m_dxgiFactory{};
	Microsoft::WRL::ComPtr<ID3D12Device> m_device{};
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandListForTransfer{};
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandListForRender{};
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocatorForTransfer{};
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocatorForRender{};
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue{};
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain{};
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_backBuffers{};
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthBuffer{};
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap{};
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap{};
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence{};
	std::vector< Microsoft::WRL::ComPtr<ID3D12Resource>> m_resourceForTransfer{};
	HANDLE m_fenceEvent{};
	UINT64 m_fenceVal = 0;
	int32_t m_backBufferWidth = 0;
	int32_t m_backBufferHeight = 0;

	D3D12_VIEWPORT m_viewport{};
	D3D12_RECT m_scissorRect{};

private: // メンバ関数

	DirectXCommon() = default;
	~DirectXCommon() = default;
	DirectXCommon(const DirectXCommon &) = delete;
	const DirectXCommon &operator=(const DirectXCommon &) = delete;

	void InitializeDXGIDevice(bool enableDebugLayer = false);

	void CreateSwapChain();

	void InitializeCommand();

	void CreateFinalRenderTargets();

	void CreateDepthBuffer();

	void CreateFence();

	void CreateViewport();

	void CreateScissor();

}; // DirectXCommon