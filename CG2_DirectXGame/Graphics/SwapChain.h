#pragma once

#include "Graphics/GpuResource.h"
#include "Graphics/GraphicsConfig.h"

#include <array>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>
#include <wrl/client.h>
#include <vector>


namespace ou {

class SwapChainOld {
public: // 静的ファクトリ

	static std::unique_ptr<SwapChainOld> Create(
		ID3D12Device *device, IDXGIFactory6 *factory, ID3D12CommandQueue *queue,
		HWND hwnd, uint32_t width, uint32_t height);


public: // メンバ関数

	// デストラクタ
	~SwapChainOld();

	// ウィンドウの大きさに合わせてバックバッファを作り直す
	void Resize(uint32_t width, uint32_t height);

	// 画面に出す
	void Present();

	// 次のフレームが描画できるようになるまで待つ
	void WaitForNextFrame();

	const GpuResourceOld &GetCurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRtv() const;
	uint32_t GetBackBufferIndex() const;
	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	void SetVSync(bool enabled);


private: // メンバ変数

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;

	std::vector<GpuResourceOld> backBuffers_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_; // TODO: アロケータを作る
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles_; // TODO: アロケータを作る

	uint32_t width_ = 0;
	uint32_t height_ = 0;

	bool vsync_ = true;
	bool tearing_ = false;
	UINT scFlags_ = 0;
	HANDLE frameLatencyWaitable_ = nullptr;

	ID3D12Device *device_ = nullptr; // TODO: デバイスのキャプチャをやめたい


private: // メンバ関数

	bool CreateBackBuffers();
	void ReleaseBackBuffers();


private: // コンストラクタ

	SwapChainOld(uint32_t width, uint32_t height);

	SwapChainOld(const SwapChainOld &) = delete;
	SwapChainOld &operator=(const SwapChainOld &) = delete;
};

} // namespace ou
