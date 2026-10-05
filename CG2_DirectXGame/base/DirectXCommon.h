#pragma once

#include "Graphics/CommandQueue.h"
#include "Graphics/D3D12Device.h"
#include "Graphics/SwapChain.h"

#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <wrl/client.h>
#include <chrono>


namespace ou {

class Window;

class DirectXCommon {
public: // 静的メンバ関数

	static DirectXCommon *GetInstance();


public: // メンバ関数

	void Initialize(Window *window, const D3D12DeviceDesc &deviceDesc = {});

	void Finalize();

	void ExecuteTransferCommands();

	void PreDraw();

	void PostDraw();

	ID3D12Device *GetDevice() const {
		return device_->Device();
	}

	ID3D12GraphicsCommandList *GetCommandListForTransfer() const {
		return commandListForTransfer_.Get();
	}

	ID3D12GraphicsCommandList *GetCommandListForRender() const {
		return commandListForRender_.Get();
	}

	size_t GetBackBufferCount() const {
		return kFrameCount;
	}


private: // メンバ変数

	std::unique_ptr<D3D12Device> device_;
	std::unique_ptr<CommandQueue> queue_;
	std::unique_ptr<SwapChain> swapChain_;

	// TODO: CommandContext に移す
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocatorForTransfer_;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocatorForRender_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandListForTransfer_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandListForRender_;

	// TODO: DepthBuffer に移す
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;

	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissorRect_{};

	std::chrono::steady_clock::time_point reference_;


private: // メンバ関数

	DirectXCommon() = default;
	~DirectXCommon() = default;
	DirectXCommon(const DirectXCommon &) = delete;
	DirectXCommon &operator=(const DirectXCommon &) = delete;

	void CreateCommandLists();
	void CreateDepthBuffer(uint32_t width, uint32_t height);
	void SetUpViewportAndScissor(uint32_t width, uint32_t height);

	// コマンドリストを閉じて実行し、GPU の完了を待ってから次に積めるようにする
	void ExecuteAndWait(ID3D12GraphicsCommandList *list, ID3D12CommandAllocator *allocator);

	void InitializeFixFPS();
	void UpdateFixFPS();
};

} // namespace ou
