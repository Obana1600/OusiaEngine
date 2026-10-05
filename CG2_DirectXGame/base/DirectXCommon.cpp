#include "DirectXCommon.h"

#include "Core/Fatal.h"
#include "Window.h"
#include <cstdint>
#include <thread>

#pragma comment(lib, "winmm.lib")


using namespace ou;
using namespace Microsoft::WRL;


// helper func ====================================================================================

namespace {

// 1/60 秒ぴったりの時間
const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
// 1/60 よりわずかに短い時間
const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

// TODO: PSO 側を kDepthFormat に揃えたら差し替える
constexpr DXGI_FORMAT kLegacyDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

void TransitionBackBuffer(
	ID3D12GraphicsCommandList *list, ID3D12Resource *resource,
	D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	list->ResourceBarrier(1, &barrier);
}

} // namespace

// ==================================================================================== helper func



// static public func =============================================================================

DirectXCommon *DirectXCommon::GetInstance() {
	static DirectXCommon instance{};
	return &instance;
}

// ============================================================================= static public func



// public func ====================================================================================

void DirectXCommon::Initialize(Window *window, const D3D12DeviceDesc &deviceDesc) {
	const auto width = static_cast<uint32_t>(window->ClientWidth());
	const auto height = static_cast<uint32_t>(window->ClientHeight());

	device_ = D3D12Device::Create(deviceDesc);
	if (!device_) { FatalError("D3D12Device の作成に失敗しました。"); }

	queue_ = CommandQueue::Create(device_->Device(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	if (!queue_) { FatalError("CommandQueue の作成に失敗しました。"); }

	swapChain_ = SwapChain::Create(
		device_->Device(), device_->Factory(), queue_->Queue(),
		window->Hwnd(), width, height);
	if (!swapChain_) { FatalError("SwapChain の作成に失敗しました。"); }

	CreateCommandLists();
	CreateDepthBuffer(width, height);
	SetUpViewportAndScissor(width, height);

	InitializeFixFPS();
}


void DirectXCommon::Finalize() {
	queue_->Flush();

	dsvHeap_.Reset();
	depthBuffer_.Reset();
	commandListForRender_.Reset();
	commandListForTransfer_.Reset();
	commandAllocatorForRender_.Reset();
	commandAllocatorForTransfer_.Reset();

	swapChain_.reset();
	queue_.reset();
	device_.reset();

	timeEndPeriod(1);
}


void DirectXCommon::PreDraw() {
	swapChain_->WaitForNextFrame();

	TransitionBackBuffer(
		commandListForRender_.Get(), swapChain_->GetCurrentBackBuffer().Resource(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	const auto rtvHandle = swapChain_->GetCurrentRtv();
	const auto dsvHandle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
	commandListForRender_->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

	constexpr float clearColor[] = {0.1f, 0.25f, 0.5f, 1.0f};
	commandListForRender_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandListForRender_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandListForRender_->RSSetViewports(1, &viewport_);
	commandListForRender_->RSSetScissorRects(1, &scissorRect_);
}


void DirectXCommon::PostDraw() {
	TransitionBackBuffer(
		commandListForRender_.Get(), swapChain_->GetCurrentBackBuffer().Resource(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	CheckHResult(commandListForRender_->Close(), "描画コマンドリストのクローズ");
	queue_->Execute(commandListForRender_.Get());

	swapChain_->Present();

	// TODO: フレームごとにアロケータを分ける
	queue_->Flush();

	UpdateFixFPS();

	CheckHResult(commandAllocatorForRender_->Reset(), "描画コマンドアロケータのリセット");
	CheckHResult(commandListForRender_->Reset(commandAllocatorForRender_.Get(), nullptr), "描画コマンドリストのリセット");
}


void DirectXCommon::ExecuteTransferCommands() {
	ExecuteAndWait(commandListForTransfer_.Get(), commandAllocatorForTransfer_.Get());
}

// ==================================================================================== public func



// private func ===================================================================================

void DirectXCommon::CreateCommandLists() {
	auto *device = device_->Device();

	CheckHResult(
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocatorForTransfer_)),
		"転送用コマンドアロケータの作成");
	CheckHResult(
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocatorForRender_)),
		"描画用コマンドアロケータの作成");

	CheckHResult(
		device->CreateCommandList(
			0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocatorForTransfer_.Get(), nullptr, IID_PPV_ARGS(&commandListForTransfer_)),
		"転送用コマンドリストの作成");
	CheckHResult(
		device->CreateCommandList(
			0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocatorForRender_.Get(), nullptr, IID_PPV_ARGS(&commandListForRender_)),
		"描画用コマンドリストの作成");

	commandListForTransfer_->SetName(L"Transfer CommandList");
	commandListForRender_->SetName(L"Render CommandList");
}


void DirectXCommon::CreateDepthBuffer(uint32_t width, uint32_t height) {
	auto *device = device_->Device();

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapDesc.NumDescriptors = 1;
	CheckHResult(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&dsvHeap_)), "DSV ヒープの作成");

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = kLegacyDepthFormat;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = kLegacyDepthFormat;
	clearValue.DepthStencil.Depth = kDepthClearValue;

	CheckHResult(
		device->CreateCommittedResource(
			&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&depthBuffer_)),
		"深度バッファの作成");
	depthBuffer_->SetName(L"Depth Buffer");

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = kLegacyDepthFormat;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(depthBuffer_.Get(), &dsvDesc, dsvHeap_->GetCPUDescriptorHandleForHeapStart());
}


void DirectXCommon::SetUpViewportAndScissor(uint32_t width, uint32_t height) {
	viewport_.TopLeftX = 0.0f;
	viewport_.TopLeftY = 0.0f;
	viewport_.Width = static_cast<float>(width);
	viewport_.Height = static_cast<float>(height);
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;

	scissorRect_.left = 0;
	scissorRect_.top = 0;
	scissorRect_.right = static_cast<LONG>(width);
	scissorRect_.bottom = static_cast<LONG>(height);
}


void DirectXCommon::ExecuteAndWait(ID3D12GraphicsCommandList *list, ID3D12CommandAllocator *allocator) {
	CheckHResult(list->Close(), "コマンドリストのクローズ");
	queue_->Execute(list);
	queue_->Flush();

	CheckHResult(allocator->Reset(), "コマンドアロケータのリセット");
	CheckHResult(list->Reset(allocator, nullptr), "コマンドリストのリセット");
}


void ou::DirectXCommon::InitializeFixFPS() {
	// 現在時間を記録する
	reference_ = std::chrono::steady_clock::now();

	timeBeginPeriod(1);
}


void ou::DirectXCommon::UpdateFixFPS() {
	// 現在時間を取得する
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	// 前回記録からの経過時間を取得する
	std::chrono::microseconds elapsed =
		std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	// 1/60 秒（よりわずかに短い時間）経っていない場合
	if (elapsed < kMinCheckTime) {
		// 1/60 秒経過するまで微小なスリープを繰り返す
		while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
			// 1 マイクロ秒スリープ
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	// 現在の時間を記録する
	reference_ = std::chrono::steady_clock::now();
}

// =================================================================================== private func
