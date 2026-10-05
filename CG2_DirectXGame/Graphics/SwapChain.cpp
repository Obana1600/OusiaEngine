#include "SwapChain.h"
#include "Core/Log.h"
#include "Graphics/GraphicsConfig.h"

using namespace ou;
using namespace Microsoft::WRL;


std::unique_ptr<SwapChain> ou::SwapChain::Create(
	ID3D12Device *device, IDXGIFactory6 *factory, ID3D12CommandQueue *queue,
	HWND hwnd, uint32_t width, uint32_t height) {
	std::unique_ptr<SwapChain> sc(new SwapChain(width, height));
	sc->device_ = device;

	// 待機可能なスワップチェーンを作る
	sc->scFlags_ |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	// ティアリングに対応しているかを調べる
	BOOL allowTearing = FALSE;
	auto hr = factory->CheckFeatureSupport(
		DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
	if (SUCCEEDED(hr)) { sc->tearing_ = (allowTearing == TRUE); }
	if (sc->tearing_) { sc->scFlags_ |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; }

	// スワップチェーンの設定
	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.Width = width;
	desc.Height = height;
	desc.Format = kBackBufferFormat;
	desc.SampleDesc = {1, 0};
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = kFrameCount;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.Flags = sc->scFlags_;

	// 下位のものを作ってから昇格
	ComPtr<IDXGISwapChain1> sc1;
	hr = factory->CreateSwapChainForHwnd(queue, hwnd, &desc, nullptr, nullptr, &sc1);
	if (FAILED(hr)) {
		Log::Error("スワップチェーンの作成に失敗しました。");
		return nullptr;
	}
	hr = sc1.As(&sc->swapChain_);
	if (FAILED(hr)) {
		Log::Error("スワップチェーンの昇格に失敗しました。");
		return nullptr;
	}

	// フレームレイテンシの設定
	hr = sc->swapChain_->SetMaximumFrameLatency(1);
	if (FAILED(hr)) {
		Log::Warning("フレームレイテンシの設定に失敗しました。");
	}
	sc->frameLatencyWaitable_ = sc->swapChain_->GetFrameLatencyWaitableObject();
	if (!sc->frameLatencyWaitable_) {
		// TODO: ログ出力
	}

	// Alt+Enter によるフルスクリーン化を禁止
	factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

	// RTV 用のヒープ
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NumDescriptors = kFrameCount;
	hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&sc->rtvHeap_));
	if (FAILED(hr)) {
		Log::Error("RTV ヒープの作成に失敗しました。");
		return nullptr;
	}

	if (!sc->CreateBackBuffers()) {
		return nullptr;
	}

	return sc;
}


ou::SwapChain::~SwapChain() {
	if (frameLatencyWaitable_) {
		CloseHandle(frameLatencyWaitable_);
	}
}


void ou::SwapChain::Resize(uint32_t width, uint32_t height) {
	if (width == 0 || height == 0) { return; }
	if (width == width_ && height == height_) { return; }

	ReleaseBackBuffers();

	auto hr = swapChain_->ResizeBuffers(kFrameCount, width, height, DXGI_FORMAT_UNKNOWN, scFlags_);
	if (FAILED(hr)) {
		Log::Error("バックバッファのリサイズに失敗しました。");
		CreateBackBuffers();
		return;
	}

	width_ = width;
	height_ = height;
	CreateBackBuffers();
}


void ou::SwapChain::Present() {
	const UINT syncInterval = vsync_ ? 1 : 0;
	const UINT presentFlags = (!vsync_ && tearing_) ? DXGI_PRESENT_ALLOW_TEARING : 0;

	auto hr = swapChain_->Present(syncInterval, presentFlags);
	if (FAILED(hr)) {
		// TODO: ログ出力
	}
}


void ou::SwapChain::WaitForNextFrame() {
	if (!frameLatencyWaitable_) { return; }

	// 1秒待つ
	auto dword = WaitForSingleObjectEx(frameLatencyWaitable_, 1000, TRUE);
	if (dword == WAIT_TIMEOUT) {
		// TODO: ログ出力
	}
}


const GpuResource &ou::SwapChain::GetCurrentBackBuffer() const {
	return backBuffers_[GetBackBufferIndex()];
}


D3D12_CPU_DESCRIPTOR_HANDLE ou::SwapChain::GetCurrentRtv() const {
	return rtvHandles_[GetBackBufferIndex()];
}


uint32_t ou::SwapChain::GetBackBufferIndex() const {
	return swapChain_->GetCurrentBackBufferIndex();
}

uint32_t ou::SwapChain::GetWidth() const {
	return width_;
}

uint32_t ou::SwapChain::GetHeight() const {
	return height_;
}

void ou::SwapChain::SetVSync(bool enabled) {
	vsync_ = enabled;
}


bool ou::SwapChain::CreateBackBuffers() {
	const auto rtvSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

	D3D12_RENDER_TARGET_VIEW_DESC desc{};
	desc.Format = kBackBufferRtvFormat;
	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	backBuffers_.reserve(kFrameCount);
	rtvHandles_.reserve(kFrameCount);

	for (UINT i = 0; i < kFrameCount; i++) {
		ComPtr<ID3D12Resource> buffer;
		auto hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&buffer));
		if (FAILED(hr)) {
			Log::Error("バックバッファの取得に失敗しました");
			return false;
		}
		device_->CreateRenderTargetView(buffer.Get(), &desc, handle);

		backBuffers_.push_back({buffer.Get(), D3D12_RESOURCE_STATE_PRESENT});
		rtvHandles_.push_back(handle);
		handle.ptr += rtvSize;
	}

	return true;
}


void ou::SwapChain::ReleaseBackBuffers() {
	backBuffers_.clear();
	rtvHandles_.clear();
}


ou::SwapChain::SwapChain(uint32_t width, uint32_t height)
	: width_(width),
	  height_(height) {}