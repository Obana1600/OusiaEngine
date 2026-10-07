#include "GraphicsDevice_DX12.h"
#include "Core/Assert.h"
#include "Core/Fatal.h"
#include "Core/Log.h"

#include <format>
#include <string>
#include <utility>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace ou;
using namespace Microsoft::WRL;


namespace {

// GPU の必要最低限の性能
constexpr D3D_FEATURE_LEVEL kMinFeatureLevel = D3D_FEATURE_LEVEL_11_0;

// QUEUE_TYPE ごとのコマンドリストの種類
constexpr D3D12_COMMAND_LIST_TYPE kCommandListTypes[QUEUE_COUNT] = {
	D3D12_COMMAND_LIST_TYPE_DIRECT,
	D3D12_COMMAND_LIST_TYPE_COMPUTE,
	D3D12_COMMAND_LIST_TYPE_COPY,
};

// コマンドキュータイプの名前
constexpr const wchar_t *kQueueNames[QUEUE_COUNT] = {
	L"QUEUE_GRAPHICS",
	L"QUEUE_COMPUTE",
	L"QUEUE_COPY",
};

// D3D12 のメッセージをログに流す
void CALLBACK OnD3D12Message(D3D12_MESSAGE_CATEGORY, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR description, void *) {
	LogLevel level = LogLevel::Debug;

	// メッセージの重大度に応じてログレベルを変更
	switch (severity) {
		case D3D12_MESSAGE_SEVERITY_CORRUPTION:
		case D3D12_MESSAGE_SEVERITY_ERROR:
			level = LogLevel::Error;
			break;

		case D3D12_MESSAGE_SEVERITY_WARNING:
			level = LogLevel::Warning;
			break;

		default:
			break;
	}

	// ログに流す
	Log::Write(level, std::source_location::current(),
			   std::format("[D3D12 #{}] {}", static_cast<int>(id), description));
}

// アダプタの選択
ComPtr<IDXGIAdapter4> SelectAdapter(IDXGIFactory6 *factory) {
	HRESULT hr{};

	// 性能順に採用
	for (UINT i = 0;; i++) {
		ComPtr<IDXGIAdapter4> candidate;
		hr = factory->EnumAdapterByGpuPreference(
			i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&candidate));
		if (hr == DXGI_ERROR_NOT_FOUND) { break; }
		if (FAILED(hr)) { continue; }

		DXGI_ADAPTER_DESC3 desc{};
		candidate->GetDesc3(&desc);

		// Microsoft WARP はスキップ
		if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) { continue; }

		// デバイスを作れるかだけ試す
		hr = D3D12CreateDevice(candidate.Get(), kMinFeatureLevel, __uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr)) { return candidate; }
	}

	// Microsoft WARP を採用する
	ComPtr<IDXGIAdapter4> warp;
	hr = factory->EnumWarpAdapter(IID_PPV_ARGS(&warp));
	if (SUCCEEDED(hr)) {
		Log::Warning("GPU が見つからなかったため WARP を使います。");
		return warp;
	}

	// アダプタを採用できなかった
	return nullptr;
}

// デバッグレイヤのメッセージの扱いの設定
void SetUpInfoQueue(ID3D12Device *device, ValidationMode validationMode, DWORD *callbackCookie) {
	ComPtr<ID3D12InfoQueue> infoQueue;
	auto hr = device->QueryInterface(IID_PPV_ARGS(&infoQueue));
	if (FAILED(hr)) { return; }

	// デバッグ中のみ
	if (IsDebuggerPresent()) {
		// メッセージの重大度がエラー以上なら止める
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
	}

	// 無視するメッセージ
	D3D12_MESSAGE_ID denyIds[] = {D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE};
	D3D12_MESSAGE_SEVERITY denySeverities[] = {D3D12_MESSAGE_SEVERITY_INFO};

	// フィルタの設定
	D3D12_INFO_QUEUE_FILTER filter{};
	filter.DenyList.NumIDs = _countof(denyIds);
	filter.DenyList.pIDList = denyIds;
	if (validationMode != ValidationMode::Verbose) {
		filter.DenyList.NumSeverities = _countof(denySeverities);
		filter.DenyList.pSeverityList = denySeverities;
	}
	infoQueue->PushStorageFilter(&filter);

	// Windos11 以降なら、メッセージをログに流す
	ComPtr<ID3D12InfoQueue1> infoQueue1;
	hr = device->QueryInterface(IID_PPV_ARGS(&infoQueue1));
	if (SUCCEEDED(hr)) {
		infoQueue1->RegisterMessageCallback(
			OnD3D12Message, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, callbackCookie);
	}
}

} // namespace



namespace ou {

// Format -> DXGI_FORMAT
constexpr DXGI_FORMAT ConvertFormat(Format format) {
	switch (format) {
		case Format::UNKNOWN:
			return DXGI_FORMAT_UNKNOWN;

		case Format::R32G32B32A32_FLOAT:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case Format::R32G32B32_FLOAT:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		case Format::R32G32_FLOAT:
			return DXGI_FORMAT_R32G32_FLOAT;
		case Format::R32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		case Format::R32_UINT:
			return DXGI_FORMAT_R32_UINT;
		case Format::R16_UINT:
			return DXGI_FORMAT_R16_UINT;

		case Format::R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Format::R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case Format::B8G8R8A8_UNORM:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case Format::B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

		case Format::D32_FLOAT:
			return DXGI_FORMAT_D32_FLOAT;
		case Format::D24_UNORM_S8_UINT:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
	}

	return DXGI_FORMAT_UNKNOWN;
}

//
constexpr DXGI_FORMAT ConvertFormatNonSrgb(DXGI_FORMAT format) {
	switch (format) {
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM;

		default:
			return format;
	}
}

struct SwapChain_DX12 {
	std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationHandler;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> backBuffers;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> backBufferRtvs;

	~SwapChain_DX12();
};

SwapChain_DX12::~SwapChain_DX12() {
	if (!allocationHandler) { return; }

	std::scoped_lock lock(allocationHandler->destroyLocker);
	const auto frameCount = allocationHandler->frameCount;
	for (auto &backBuffer : backBuffers) {
		allocationHandler->destroyerResources.push_back({backBuffer, frameCount});
	}
}

SwapChain_DX12 *ToInternal(const SwapChain *param) {
	return static_cast<SwapChain_DX12 *>(param->internalState.get());
}

} // namespace ou



ou::GraphicsDevice_DX12::GraphicsDevice_DX12(ValidationMode validationMode)
	: validationMode_(validationMode) {
	HRESULT hr{};

	// ----- デバッグ機能 -----
	if (validationMode_ != ValidationMode::Disabled) {
		// デバッグレイヤ
		ComPtr<ID3D12Debug1> debug;
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
		if (SUCCEEDED(hr)) {
			debug->EnableDebugLayer();

			// GPU ベース検証
			if (validationMode_ == ValidationMode::Gpu) {
				debug->SetEnableGPUBasedValidation(TRUE);
				debug->SetEnableSynchronizedCommandQueueValidation(TRUE);
			}

		} else {
			Log::Warning("デバッグレイヤーの有効化に失敗しました。");
			validationMode_ = ValidationMode::Disabled;
		}

		// DRED
		ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dred;
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dred));
		if (SUCCEEDED(hr)) {
			dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			dred->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		}
	}

	// ----- ファクトリ -----
	UINT factoryFlags = (validationMode_ != ValidationMode::Disabled) ? DXGI_CREATE_FACTORY_DEBUG : 0;
	hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory_));
	if (FAILED(hr) && factoryFlags != 0) {
		Log::Warning("DXGI のデバッグ機能の有効化に失敗しました。");
		hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory_));
	}
	CheckHResult(hr, "DXGI ファクトリの作成");

	// ティアリングに対応しているか
	BOOL allowTearing = FALSE;
	hr = factory_->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
	tearingSupported_ = SUCCEEDED(hr) && allowTearing;

	// ----- アダプタ / デバイス -----
	adapter_ = SelectAdapter(factory_.Get());
	if (!adapter_) { FatalError("D3D12 を使えるアダプタが見つかりませんでした。"); }

	hr = D3D12CreateDevice(adapter_.Get(), kMinFeatureLevel, IID_PPV_ARGS(&device_));
	CheckHResult(hr, "D3D12 デバイスの作成");
	device_->SetName(L"GraphicsDevice_DX12");

	if (validationMode_ != ValidationMode::Disabled) {
		SetUpInfoQueue(device_.Get(), validationMode_, &messageCallbackCookie_);
	}

	// ----- コマンドキュー -----
	for (UINT i = 0; i < QUEUE_COUNT; i++) {
		auto &queue = queues_[i];
		queue.desc.Type = kCommandListTypes[i];
		queue.desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queue.desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queue.desc.NodeMask = 0;

		hr = device_->CreateCommandQueue(&queue.desc, IID_PPV_ARGS(&queue.queue));
		CheckHResult(hr, "コマンドキューの作成");
		queue.queue->SetName(kQueueNames[i]);
	}

	// ----- フェンス -----
	for (uint32_t b = 0; b < BUFFER_COUNT; b++) {
		for (uint32_t q = 0; q < QUEUE_COUNT; q++) {
			hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFence_[b][q]));
			CheckHResult(hr, "フレームフェンスの作成");
		}
	}

	// ----- 遅延破棄 -----
	allocationHandler_ = std::make_shared<AllocationHandler>();

	Log::Info("GraphicsDevice_DX12 を作成しました。");
}


ou::GraphicsDevice_DX12::~GraphicsDevice_DX12() {
	if (!device_) { return; }

	WaitForGpu();

	HRESULT hr{};

	//
	ComPtr<ID3D12InfoQueue> infoQueue;
	hr = device_.As(&infoQueue);
	if (SUCCEEDED(hr)) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, FALSE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, FALSE);
	}

	//
	if (messageCallbackCookie_ != 0) {
		ComPtr<ID3D12InfoQueue1> infoQueue1;
		hr = device_.As(&infoQueue1);
		if (SUCCEEDED(hr)) {
			infoQueue1->UnregisterMessageCallback(messageCallbackCookie_);
		}
	}
}


bool ou::GraphicsDevice_DX12::CreateSwapChain(const SwapChainDesc *desc, HWND hwnd, SwapChain *swapChain) {
	HRESULT hr{};

	if (desc->width == 0 || desc->height == 0) { return false; }

	auto scDX12 = std::static_pointer_cast<SwapChain_DX12>(swapChain->internalState);
	if (!scDX12) {
		scDX12 = std::make_shared<SwapChain_DX12>();
	}
	scDX12->allocationHandler = allocationHandler_;
	swapChain->internalState = scDX12;
	swapChain->desc = *desc;

	UINT flags = 0;
	if (tearingSupported_) { flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; }

	const auto rtvFormat = ConvertFormat(desc->format);
	const auto bufferFormat = ConvertFormatNonSrgb(rtvFormat);

	// 新しく作る
	if (!scDX12->swapChain) {
		DXGI_SWAP_CHAIN_DESC1 scDesc{};
		scDesc.Width = desc->width;
		scDesc.Height = desc->height;
		scDesc.Format = bufferFormat;
		scDesc.Stereo = FALSE;
		scDesc.SampleDesc = {1, 0};
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.BufferCount = desc->bufferCount;
		scDesc.Scaling = DXGI_SCALING_STRETCH;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		scDesc.Flags = flags;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc{};
		fsDesc.Windowed = !desc->fullscreen;

		ComPtr<IDXGISwapChain1> swapChain1;
		hr = factory_->CreateSwapChainForHwnd(
			queues_[QUEUE_GRAPHICS].queue.Get(), hwnd, &scDesc, &fsDesc, nullptr, &swapChain1);
		if (FAILED(hr)) {
			Log::Error("スワップチェーンの作成に失敗しました。");
			return false;
		}

		// Alt+Enter でのフルスクリーン化を無効にする
		factory_->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

		hr = swapChain1.As(&scDX12->swapChain);
		if (FAILED(hr)) {
			Log::Error("IDXGISwapChain4 の取得に失敗しました。");
			return false;
		}

	} else {
		// リサイズ
		WaitForGpu();
		scDX12->backBuffers.clear();
		scDX12->backBufferRtvs.clear();

		hr = scDX12->swapChain->ResizeBuffers(
			desc->bufferCount, desc->width, desc->height, bufferFormat, flags);
		if (FAILED(hr)) {
			Log::Error("スワップチェーンのリサイズに失敗しました。");
			return false;
		}
	}

	// バックバッファの RTV を作成する
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NumDescriptors = desc->bufferCount;
	hr = device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&scDX12->rtvHeap));
	if (FAILED(hr)) {
		Log::Error("RTV ヒープの作成に失敗しました。");
		return false;
	}

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = rtvFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	const auto rtvSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto handle = scDX12->rtvHeap->GetCPUDescriptorHandleForHeapStart();

	scDX12->backBuffers.resize(desc->bufferCount);
	scDX12->backBufferRtvs.resize(desc->bufferCount);
	for (uint32_t i = 0; i < desc->bufferCount; i++) {
		hr = scDX12->swapChain->GetBuffer(i, IID_PPV_ARGS(&scDX12->backBuffers[i]));
		if (FAILED(hr)) {
			Log::Error("バックバッファの取得に失敗しました。");
			return false;
		}

		device_->CreateRenderTargetView(scDX12->backBuffers[i].Get(), &rtvDesc, handle);
		scDX12->backBufferRtvs[i] = handle;
		handle.ptr += rtvSize;
	}

	return true;
}


bool ou::GraphicsDevice_DX12::CreateBuffer(const GpuBufferDesc *desc, const void *initialData, GpuBuffer *buffer) {
	(void)desc;
	(void)initialData;
	(void)buffer;
	return false;
}


bool ou::GraphicsDevice_DX12::CreateTexture(const TextureDesc *desc, const SubresourceData *initialData, Texture *texture) {
	(void)desc;
	(void)initialData;
	(void)texture;
	return false;
}


bool ou::GraphicsDevice_DX12::CreateShader(ShaderStage stage, const void *shaderCode, size_t shaderCodeSize, Shader *shader) {
	(void)stage;
	(void)shaderCode;
	(void)shaderCodeSize;
	(void)shader;
	return false;
}


bool ou::GraphicsDevice_DX12::CreateSampler(const SamplerDesc *desc, Sampler *sampler) {
	(void)desc;
	(void)sampler;
	return false;
}


bool ou::GraphicsDevice_DX12::CreatePipelineState(const PipelineStateDesc *desc, PipelineState *pso) {
	(void)desc;
	(void)pso;
	return false;
}


void ou::GraphicsDevice_DX12::SetName(GpuResource *resource, std::string_view name) const {
	(void)resource;
	(void)name;
}


CommandList ou::GraphicsDevice_DX12::BeginCommandList(QueueType queue) {
	HRESULT hr{};

	// スロットから 1 つ借りる
	CommandList cmd;
	{
		std::scoped_lock lock(cmdLocker_);

		const auto cmdCurrent = cmdCount_++;
		if (cmdCurrent >= commandLists_.size()) {
			commandLists_.push_back(std::make_unique<CommandList_DX12>());
		}
		cmd.internalState = commandLists_[cmdCurrent].get();
	}

	auto &cmdDX12 = GetCommandList(cmd);
	cmdDX12.Reset(GetBufferIndex());
	cmdDX12.queue = queue;
	cmdDX12.id = cmdCount_ - 1;

	// このキュータイプのリストがなければ作る
	if (!cmdDX12.GetGraphicsCommandList()) {
		const auto type = queues_[queue].desc.Type;

		for (uint32_t b = 0; b < BUFFER_COUNT; b++) {
			hr = device_->CreateCommandAllocator(
				type, IID_PPV_ARGS(&cmdDX12.allocators[b][queue]));
			CheckHResult(hr, "コマンドアロケータの作成");
		}

		hr = device_->CreateCommandList(
			0, type, cmdDX12.allocators[0][queue].Get(), nullptr, IID_PPV_ARGS(&cmdDX12.lists[queue]));
		CheckHResult(hr, "コマンドリストの作成");

		// リストを閉じる
		// TODO: Device4? で作れば Close 状態で作れる
		cmdDX12.lists[queue]->Close();

		std::wstring name = L"CommandList " + std::to_wstring(cmdDX12.id);
		cmdDX12.lists[queue]->SetName(name.c_str());
	}

	// 記録を始める
	hr = cmdDX12.GetCommandAllocator()->Reset();
	CheckHResult(hr, "コマンドアロケータのリセット");
	hr = cmdDX12.GetGraphicsCommandList()->Reset(cmdDX12.GetCommandAllocator(), nullptr);
	CheckHResult(hr, "コマンドリストのリセット");

	return cmd;
}


void ou::GraphicsDevice_DX12::SubmitCommandLists() {
	HRESULT hr{};

	std::scoped_lock lock(cmdLocker_);

	// ----- 現在フレームのリストを閉じて、キューに詰める -----
	const auto cmdLast = cmdCount_;
	cmdCount_ = 0;
	std::vector<const SwapChain *> swapChainToPresent;

	for (uint32_t i = 0; i < cmdLast; i++) {
		auto &cmdDX12 = *commandLists_[i];

		hr = cmdDX12.GetGraphicsCommandList()->Close();
		CheckHResult(hr, "コマンドリストのクローズ");

		queues_[cmdDX12.queue].submitCmds.push_back(cmdDX12.GetGraphicsCommandList());

		for (const auto *sc : cmdDX12.swapChains) {
			if (std::find(swapChainToPresent.begin(), swapChainToPresent.end(), sc) == swapChainToPresent.end()) {
				swapChainToPresent.push_back(sc);
			}
		}
	}

	// ----- 実行する -----
	for (int q = 0; q < QUEUE_COUNT; q++) {
		queues_[q].Submit();

		hr = queues_[q].queue->Signal(frameFence_[GetBufferIndex()][q].Get(), 1);
		CheckHResult(hr, "フレームフェンスのシグナル");
	}

	// ----- Present する -----
	for (const auto *sc : swapChainToPresent) {
		auto *scDX12 = ToInternal(sc);

		const auto vsync = sc->desc.vsync;
		const UINT syncInterval = vsync ? 1 : 0;
		const UINT presentFlags = (!vsync && tearingSupported_) ? DXGI_FEATURE_PRESENT_ALLOW_TEARING : 0;

		hr = scDX12->swapChain->Present(syncInterval, presentFlags);
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
			FatalError(std::format("デバイスが失われました: {}", DescribeHResult(device_->GetDeviceRemovedReason())));
		}
	}

	// フレームを進める
	frameCount_++;

	// GPU 処理が終わるまで待つ
	const auto bufferIndex = GetBufferIndex();
	for (int q = 0; q < QUEUE_COUNT; q++) {
		auto *fence = frameFence_[bufferIndex][q].Get();

		if (frameCount_ >= BUFFER_COUNT && fence->GetCompletedValue() < 1) {
			hr = fence->SetEventOnCompletion(1, nullptr);
			CheckHResult(hr, "フレームフェンスの待機");
		}
		fence->Signal(0);
	}

	// 使ったものを解放する
	allocationHandler_->Update(frameCount_, BUFFER_COUNT);
}


void ou::GraphicsDevice_DX12::WaitForGpu() const {
	ComPtr<ID3D12Fence> fence;
	HRESULT hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	CheckHResult(hr, "待機用フェンスの作成");

	for (const auto &queue : queues_) {
		hr = queue.queue->Signal(fence.Get(), 1);
		CheckHResult(hr, "待機用フェンスのシグナル");

		if (fence->GetCompletedValue() < 1) {
			hr = fence->SetEventOnCompletion(1, nullptr);
			CheckHResult(hr, "待機用フェンスの待機");
		}
		fence->Signal(0);
	}
}


void ou::GraphicsDevice_DX12::RenderPassBegin(const SwapChain *swapChain, CommandList cmd) {
	auto &cmdDX12 = GetCommandList(cmd);
	auto *list = cmdDX12.GetGraphicsCommandList();
	auto *scDX12 = ToInternal(swapChain);

	cmdDX12.swapChains.push_back(swapChain);

	const auto backBufferIndex = scDX12->swapChain->GetCurrentBackBufferIndex();
	auto *backBuffer = scDX12->backBuffers[backBufferIndex].Get();
	const auto rtv = scDX12->backBufferRtvs[backBufferIndex];

	// PRESENT -> RENDER_TARGET
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = backBuffer;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	list->ResourceBarrier(1, &barrier);

	// TODO: CommandList4 なら BeginRenderPass でできる
	list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
	list->ClearRenderTargetView(rtv, swapChain->desc.clearColor, 0, nullptr);

	std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
	cmdDX12.renderPassBarriersEnd.push_back(barrier);
}


void ou::GraphicsDevice_DX12::RenderPassBegin(const RenderPassImage *images, uint32_t imageCount, CommandList cmd) {
	(void)images;
	(void)imageCount;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::RenderPassEnd(CommandList cmd) {
	auto &cmdDX12 = GetCommandList(cmd);
	auto &barriers = cmdDX12.renderPassBarriersEnd;

	if (!barriers.empty()) {
		cmdDX12.GetGraphicsCommandList()->ResourceBarrier(
			static_cast<UINT>(barriers.size()), barriers.data());
		barriers.clear();
	}
	(void)cmd;
}


void ou::GraphicsDevice_DX12::BindViewports(uint32_t numViewports, const Viewport *viewports, CommandList cmd) {
	(void)numViewports;
	(void)viewports;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::BindScissorRects(uint32_t numScissors, const Scissor *scissors, CommandList cmd) {
	(void)numScissors;
	(void)scissors;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::BindPipelineState(const PipelineState *pso, CommandList cmd) {
	(void)pso;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::BindVertexBuffers(const GpuBuffer *const *buffers, uint32_t slot, uint32_t count, const uint32_t *strides, const uint64_t *offsets, CommandList cmd) {
	(void)buffers;
	(void)slot;
	(void)count;
	(void)strides;
	(void)offsets;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::BindIndexBuffer(const GpuBuffer *buffer, IndexBufferFormat format, uint64_t offsets, CommandList cmd) {
	(void)buffer;
	(void)format;
	(void)offsets;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::BindConstantBuffer(const GpuBuffer *buffer, uint32_t slot, CommandList cmd, uint64_t offset) {
	(void)buffer;
	(void)slot;
	(void)cmd;
	(void)offset;
}


void ou::GraphicsDevice_DX12::BindResource(const GpuResource *resource, uint32_t slot, CommandList cmd, int subresource) {
	(void)resource;
	(void)slot;
	(void)cmd;
	(void)subresource;
}


void ou::GraphicsDevice_DX12::BindSampler(const Sampler *sampler, uint32_t slot, CommandList cmd) {
	(void)sampler;
	(void)slot;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd) {
	(void)vertexCount;
	(void)startVertexLocation;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd) {
	(void)indexCount;
	(void)startIndexLocation;
	(void)baseVertexLocation;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) {
	(void)vertexCount;
	(void)instanceCount;
	(void)startInstanceLocation;
	(void)startVertexLocation;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd) {
	(void)indexCount;
	(void)instanceCount;
	(void)startIndexLocation;
	(void)baseVertexLocation;
	(void)startInstanceLocation;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::Barrier(const GpuBarrier *barriers, uint32_t numBarriers, CommandList cmd) {
	(void)barriers;
	(void)numBarriers;
	(void)cmd;
}


GraphicsDevice_DX12::CommandList_DX12 &ou::GraphicsDevice_DX12::GetCommandList(CommandList cmd) const {
	ASSERT(cmd.IsValid());
	return *static_cast<CommandList_DX12 *>(cmd.internalState);
}


void ou::GraphicsDevice_DX12::CommandQueue::Submit() {
	if (submitCmds.empty()) { return; }

	queue->ExecuteCommandLists(static_cast<UINT>(submitCmds.size()), submitCmds.data());
	submitCmds.clear();
}


void ou::GraphicsDevice_DX12::CommandList_DX12::Reset(uint32_t newBufferIndex) {
	bufferIndex = newBufferIndex;
	swapChains.clear();
	renderPassBarriersEnd.clear();
}


ID3D12CommandAllocator *ou::GraphicsDevice_DX12::CommandList_DX12::GetCommandAllocator() const {
	return allocators[bufferIndex][queue].Get();
}


ID3D12GraphicsCommandList *ou::GraphicsDevice_DX12::CommandList_DX12::GetGraphicsCommandList() const {
	return lists[queue].Get();
}


void ou::GraphicsDevice_DX12::AllocationHandler::Update(uint64_t currentFrameCount, uint32_t bufferCount) {
	std::scoped_lock lock(destroyLocker);
	frameCount = currentFrameCount;

	auto release = [&](auto &destroyer) {
		while (!destroyer.empty() && destroyer.front().second + bufferCount < frameCount) {
			destroyer.pop_front();
		}
	};
	release(destroyerResources);
	release(destroyerPipelines);
}
