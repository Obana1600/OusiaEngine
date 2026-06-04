#include "DirectXCommon.h"
#include "WinApp.h"
#include <cassert>

using namespace Microsoft::WRL;


// helper func ====================================================================================

namespace {

	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		const ComPtr<ID3D12Device> &device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors
	) {
		// ディスクリプタヒープの生成
		ComPtr<ID3D12DescriptorHeap> descriptorHeap{};
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
		descriptorHeapDesc.Type = heapType;
		descriptorHeapDesc.NumDescriptors = numDescriptors;
		descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
		// ディスクリプタヒープが作れなかったので起動できない
		assert(SUCCEEDED(hr));

		return descriptorHeap;
	}


	ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(const ComPtr<ID3D12Device> &device, int32_t width, int32_t height) {
		// 生成するResourceの設定
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Width = width;										// Textureの幅
		resourceDesc.Height = height;									// Textureの高さ
		resourceDesc.MipLevels = 1;										// mipmapの数
		resourceDesc.DepthOrArraySize = 1;								// 奥行き or 配列Textureの配列数
		resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;			// DepthStencilとして利用可能なフォーマット
		resourceDesc.SampleDesc.Count = 1;								// サンプリングカウント。1固定
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 2次元
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// DepthStencilとして使う通知

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
			&heapProperties,					// Heapの設定
			D3D12_HEAP_FLAG_NONE,				// Heapの特殊な設定。特になし
			&resourceDesc,						// Resourceの設定
			D3D12_RESOURCE_STATE_DEPTH_WRITE,	// 深度地を書き込む状態にしておく
			&depthClearValue,					// Clear最適値
			IID_PPV_ARGS(&resource)				// 作成するResourceポインタへのポインタ
		);
		assert(SUCCEEDED(hr));

		return resource;
	}

}

// ==================================================================================== helper func



// static public func =============================================================================

DirectXCommon *DirectXCommon::GetInstance() {
	static DirectXCommon instance{};
	return &instance;
}

// ============================================================================= static public func



// public func ====================================================================================

void DirectXCommon::Initialize(int32_t backBufferWidth, int32_t backBufferHeight, bool enableDebugLayer) {
	m_backBufferWidth = backBufferWidth;
	m_backBufferHeight = backBufferHeight;


	// factory -----------------------------------------------------------

	// HRESULTはwindows系のエラーコードであり、
	// 関数が成功したかどうかをSUCCEEDEDマクロで判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&m_dxgiFactory));
	// 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、どうにもできない場合が多いのでassertにしておく
	assert(SUCCEEDED(hr));

	// -------------------------------------------------------------------

	InitializeDXGIDevice(enableDebugLayer);

	InitializeCommand();

	CreateSwapChain();

	CreateFinalRenderTargets();

	CreateDepthBuffer();

	CreateFence();

	CreateViewport();

	CreateScissor();
}


void DirectXCommon::Finalize() {
	CloseHandle(m_fenceEvent);
}


void DirectXCommon::PreDraw() {
	// これから書き込むバックバッファのインデックスを取得
	UINT backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

	// TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;					// 今回のバリアはTransition
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;						// Noneにしておく
	barrier.Transition.pResource = m_backBuffers[backBufferIndex].Get();	// バリアを張る対象のリソース。現在のバックバッファに対して行う
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;			// 遷移前（現在）のResourceState
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;		// 遷移後のResourceState
	m_commandListForRender->ResourceBarrier(1, &barrier);					// TransitionBarrierを張る

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	// RTVハンドルを計算
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * backBufferIndex;

	m_commandListForRender->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

	constexpr float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f }; // 青っぽい色。RGBAの順
	m_commandListForRender->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr); // 指定した色で画面全体をクリア
	m_commandListForRender->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // 指定した深度で画面全体をクリア

	m_commandListForRender->RSSetViewports(1, &m_viewport); // Viewportを設定
	m_commandListForRender->RSSetScissorRects(1, &m_scissorRect); // Scissorを設定
}


void DirectXCommon::PostDraw() {
	// これから書き込むバックバッファのインデックスを取得
	UINT backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

	// TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;					// 今回のバリアはTransition
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;						// Noneにしておく
	barrier.Transition.pResource = m_backBuffers[backBufferIndex].Get();	// バリアを張る対象のリソース。現在のバックバッファに対して行う
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;	// 遷移前（現在）のResourceState
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;			// 遷移後のResourceState
	m_commandListForRender->ResourceBarrier(1, &barrier);					// TransitionBarrierを張る

	// コマンドリストの内容を確定させる。すべてのコマンドを積んでからCloseすること
	HRESULT hr = m_commandListForRender->Close();
	assert(SUCCEEDED(hr));

	// GPUにコマンドリストの実行を行わせる
	ID3D12CommandList *commandLists[] = { m_commandListForRender.Get() };
	m_commandQueue->ExecuteCommandLists(1, commandLists);
	// GPUとOSに画面の交換を行うよう通知する
	m_swapChain->Present(1, 0);

	// Fenceの値を更新
	m_fenceVal++;
	// GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
	m_commandQueue->Signal(m_fence.Get(), m_fenceVal);
	// Fenceの値が指定したSignal値にたどり着いているか確認する
	// GetCompleteValueの初期値はFence作成時に渡した初期値
	if (m_fence->GetCompletedValue() < m_fenceVal) {
		// 指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
		m_fence->SetEventOnCompletion(m_fenceVal, m_fenceEvent);
		// イベント待つ
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	// 次のフレーム用のコマンドリストを準備
	hr = m_commandAllocatorForRender->Reset();
	assert(SUCCEEDED(hr));
	hr = m_commandListForRender->Reset(m_commandAllocatorForRender.Get(), nullptr);
	assert(SUCCEEDED(hr));
}

// ==================================================================================== public func



// private func ===================================================================================

void DirectXCommon::InitializeDXGIDevice(bool enableDebugLayer) {
	HRESULT hr{};

	// デバイス生成前
	if (enableDebugLayer) {
		ComPtr<ID3D12Debug1> debugController{};
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			// デバッグレイヤを有効化する
			debugController->EnableDebugLayer();
			// さらにGUI側でもチェックを行うようにする
			debugController->SetEnableGPUBasedValidation(TRUE);
		}
	}


	// device ------------------------------------------------------------

	// 使用するアダプタ用の変数
	ComPtr<IDXGIAdapter4> useAdapter{};
	// 良い順にアダプタを頼む
	for (UINT i = 0;
		m_dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND;
		i++) {
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc); // アダプタの情報を取得する
		assert(SUCCEEDED(hr));

		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) { // ソフトウェアアダプタでなければ採用！
			break;
		}
		useAdapter = nullptr; // ソフトウェアアダプタの場合は見なかったことにする
	}
	// 適切なアダプタが見つからなかったので起動できない
	assert(useAdapter);


	// 機能レベルとログ出力用の文字列
	constexpr D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0
	};
	// 高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); i++) {
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&m_device)); // 採用したアダプターでデバイスを生成
		if (SUCCEEDED(hr)) { // 指定した機能レベルでデバイスが生成できたかを確認
			break;
		}
	}
	// デバイスの生成がうまくいかなかったので起動できない
	assert(m_device);

	// -------------------------------------------------------------------


	// debugLayer --------------------------------------------------------

	if (enableDebugLayer) {
		ComPtr<ID3D12InfoQueue> infoQueue{};
		if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true); // ヤバいエラー時に止まる
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true); // エラー時に止まる
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true); // 警告時に止まる
			// 抑制するメッセージのID
			D3D12_MESSAGE_ID denyIds[] = {
				// Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
				D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
			};
			// 抑制するレベル
			D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
			D3D12_INFO_QUEUE_FILTER filter{};
			filter.DenyList.NumIDs = _countof(denyIds);
			filter.DenyList.pIDList = denyIds;
			filter.DenyList.NumSeverities = _countof(severities);
			filter.DenyList.pSeverityList = severities;
			// 指定したメッセージの表示を抑制する
			infoQueue->PushStorageFilter(&filter);
		}
	}

	// -------------------------------------------------------------------
}


void DirectXCommon::CreateSwapChain() {
	constexpr uint32_t backBufferCount = 2;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = m_backBufferWidth;						// 画面の幅。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Height = m_backBufferHeight;						// 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				// 色の形式(RGBA)
	swapChainDesc.SampleDesc.Count = 1;								// マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// マルチサンプルしない
	swapChainDesc.BufferCount = backBufferCount;					// バッファ数
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// モニタに写したら、中身を破棄
	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	HRESULT hr = m_dxgiFactory->CreateSwapChainForHwnd(
		m_commandQueue.Get(), WinApp::GetInstance()->GetHwnd(), &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1 **>(m_swapChain.ReleaseAndGetAddressOf()));
	assert(SUCCEEDED(hr));
}


void DirectXCommon::InitializeCommand() {
	// commandQueue ------------------------------------------------------

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	HRESULT hr = m_device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_commandQueue));
	assert(SUCCEEDED(hr));

	// -------------------------------------------------------------------


	// commandAllocator --------------------------------------------------

	hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocatorForTransfer));
	assert(SUCCEEDED(hr));

	hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocatorForRender));
	assert(SUCCEEDED(hr));

	// -------------------------------------------------------------------


	// commandList -------------------------------------------------------

	hr = m_device->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorForTransfer.Get(), nullptr, IID_PPV_ARGS(&m_commandListForTransfer));
	assert(SUCCEEDED(hr));

	hr = m_device->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorForRender.Get(), nullptr, IID_PPV_ARGS(&m_commandListForRender));
	assert(SUCCEEDED(hr));

	// -------------------------------------------------------------------
}


void DirectXCommon::CreateFinalRenderTargets() {
	// バッファ数の取得
	DXGI_SWAP_CHAIN_DESC1 desc{};
	m_swapChain->GetDesc1(&desc);
	uint32_t backBufferCount = desc.BufferCount;

	// RTV用のヒープでディスクリプタの数は2
	m_rtvHeap = CreateDescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, backBufferCount);

	// バックバッファ
	m_backBuffers.resize(backBufferCount);
	for (uint32_t i = 0; i < m_backBuffers.size(); i++) {
		HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
		assert(SUCCEEDED(hr));
	}

	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;		// 出力結果をSRGBに変換して書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;	// 2Dテクスチャとして書き込む
	// ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	uint32_t rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// ディスクリプタを順番に作る
	for (uint32_t i = 0; i < m_backBuffers.size(); i++) {
		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvStartHandle;
		handle.ptr += rtvDescriptorSize * i;
		m_device->CreateRenderTargetView(m_backBuffers[i].Get(), &rtvDesc, handle);
	}
}


void DirectXCommon::CreateDepthBuffer() {
	// DSV用のヒープでディスクリプタの数は1
	m_dsvHeap = CreateDescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);

	// DepthStencilTextureをウィンドウのサイズで作成
	m_depthBuffer = CreateDepthStencilTextureResource(m_device, m_backBufferWidth, m_backBufferHeight);
	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;			// Format。基本的にはResourceに合わせる
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// 2dTexture
	// DSVHeapの先頭にDSVを作る
	m_device->CreateDepthStencilView(m_depthBuffer.Get(), &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
}


void DirectXCommon::CreateFence() {
	HRESULT hr = m_device->CreateFence(m_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
	assert(SUCCEEDED(hr));

	// FenceのSignalを待つためのイベントを作成する
	m_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_fenceEvent != nullptr);
}


void DirectXCommon::CreateViewport() {
	// クライアント領域のサイズと一緒にして画面全体に表示
	m_viewport.Width = WinApp::kWindowWidth;
	m_viewport.Height = WinApp::kWindowHeight;
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
}


void DirectXCommon::CreateScissor() {
	// 基本的にビューポートと同じ矩形が構成されるようにする
	m_scissorRect.left = 0;
	m_scissorRect.right = WinApp::kWindowWidth;
	m_scissorRect.top = 0;
	m_scissorRect.bottom = WinApp::kWindowHeight;
}

// =================================================================================== private func