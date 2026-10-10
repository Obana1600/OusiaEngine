#include "D3D12Device.h"
#include "Core/Assert.h"
#include "Core/Fatal.h"
#include "Core/Log.h"

#include <format>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace ou;
using namespace Microsoft::WRL;


namespace {

// GPU の必要最低限の性能
constexpr D3D_FEATURE_LEVEL kMinFeatureLevel = D3D_FEATURE_LEVEL_11_0;

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

} // namespace



std::unique_ptr<D3D12Device> D3D12Device::Create(const D3D12DeviceDesc &desc) {
	std::unique_ptr<D3D12Device> d(new D3D12Device());
	d->desc_ = desc;
	auto &dd = d->desc_;

	if (!dd.enableDebugLayer) { dd.enableGpuBasedValidation = false; }

	if (dd.enableDebugLayer) {
		if (!d->EnableDebugLayer()) {
			Log::Debug("デバッグレイヤーの有効化に失敗しました。");
			dd.enableDebugLayer = false;
			dd.enableGpuBasedValidation = false;
		}
	}

	if (desc.enableDred) {
		if (!d->EnableDred()) {
			Log::Debug("DRED の有効化に失敗しました。");
			dd.enableDred = false;
		}
	}

	if (!d->CreateFactory()) {
		Log::Error("DXGI ファクトリの生成に失敗しました。");
		return nullptr;
	}

	if (!d->SelectAdapter()) {
		Log::Error("GPU が見つかりませんでした。");
		return nullptr;
	}

	if (!d->CreateDevice()) {
		Log::Error("デバイスの作成に失敗しました。");
		return nullptr;
	}

	if (dd.enableDebugLayer) { d->SetUpInfoQueue(); }

	return d;
}


ou::D3D12Device::~D3D12Device() {
	if (!device_) { return; }
	HRESULT hr{};

	// コールバックの登録を先に解除する
	if (messageCallbackCockie_ != 0) {
		ComPtr<ID3D12InfoQueue1> infoQueue1;
		hr = device_.As(&infoQueue1);
		if (SUCCEEDED(hr)) {
			infoQueue1->UnregisterMessageCallback(messageCallbackCockie_);
		}
		messageCallbackCockie_ = 0;
	}

	// 
	ComPtr<ID3D12InfoQueue> infoQueue;
	hr = device_.As(&infoQueue);
	if (SUCCEEDED(hr)) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, FALSE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, FALSE);
	}
}


HRESULT ou::D3D12Device::GetDeviceRemovedReason() const {
	// デバイス未初期化
	if (!device_) { return E_FAIL; }

	return device_->GetDeviceRemovedReason();
}


void ou::D3D12Device::LogDeviceRemovedInfo() const {
	const auto reason = GetDeviceRemovedReason();

	// ロストしていない
	if (reason == S_OK || reason == E_FAIL) { return; }

	// TODO: ログ出力

	if (!desc_.enableDred) { return; }
	ComPtr<ID3D12DeviceRemovedExtendedData1> dred;
	auto hr = device_.As(&dred);
	if (FAILED(hr)) { return; }

	// GPU がどのコマンドまで実行し終えていたか
	D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbs{};
	hr = dred->GetAutoBreadcrumbsOutput1(&breadcrumbs);
	if (SUCCEEDED(hr)) {
		for (auto *node = breadcrumbs.pHeadAutoBreadcrumbNode; node; node = node->pNext) {
			const auto completed = node->pLastBreadcrumbValue ? *node->pLastBreadcrumbValue : 0;
			// 全操作を完了したコマンドリストは飛ばす
			if (completed >= node->BreadcrumbCount) { continue; }

			// TODO: ログ出力

			// 失敗処理付近の操作を出力
			for (UINT i = 0; i < node->BreadcrumbCount; i++) {
				// TODO: ログ出力
			}
		}
	}

	// どの GPU アドレスで不正なアクセスが起きたか
	D3D12_DRED_PAGE_FAULT_OUTPUT1 pageFault{};
	hr = dred->GetPageFaultAllocationOutput1(&pageFault);
	if (SUCCEEDED(hr) && pageFault.PageFaultVA != 0) {
		// TODO: ログ出力

		for (auto *a = pageFault.pHeadExistingAllocationNode; a; a = a->pNext) {
			// TODO: ログ出力
		}
		for (auto *a = pageFault.pHeadRecentFreedAllocationNode; a; a = a->pNext) {
			// TODO: ログ出力
		}
	}
}


IDXGIFactory6 *ou::D3D12Device::Factory() const {
	return factory_.Get();
}


IDXGIAdapter4 *ou::D3D12Device::Adapter() const {
	return adapter_.Get();
}


ID3D12Device *ou::D3D12Device::Device() const {
	return device_.Get();
}


bool ou::D3D12Device::EnableDebugLayer() {
	ComPtr<ID3D12Debug> debug;
	auto hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));

	if (SUCCEEDED(hr)) {
		debug->EnableDebugLayer();

		// GPU 検証
		if (desc_.enableGpuBasedValidation) {
			ComPtr<ID3D12Debug1> debug1;
			hr = debug.As(&debug1);
			if (SUCCEEDED(hr)) {
				debug1->SetEnableGPUBasedValidation(TRUE);
			}
		}

		return true;
	}

	return false;
}


bool ou::D3D12Device::EnableDred() {
	ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dred;
	auto hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dred));

	if (SUCCEEDED(hr)) {
		// GPU が最後に実行していたコマンドの履歴
		dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		// ページフォールトのアドレスの記録
		dred->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

		return true;
	}

	return false;
}


bool ou::D3D12Device::CreateFactory() {
	// デバッグレイヤー有効時は DXGI のデバッグ機能も有効にする
	UINT flags = desc_.enableDebugLayer ? DXGI_CREATE_FACTORY_DEBUG : 0;

	auto hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory_));
	if (SUCCEEDED(hr)) {
		return true;
	}

	// 失敗時はログに流して、デバッグ機能なしで作り直す
	if (FAILED(hr) && flags != 0) {
		hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory_));
		if (SUCCEEDED(hr)) {
			Log::Warning("ファクトリのデバッグ機能有効化に失敗しました");
			return true;
		}
	}

	return false;
}


bool ou::D3D12Device::SelectAdapter() {
	HRESULT hr{};

	// 性能順に採用
	for (UINT i = 0;; i++) {
		ComPtr<IDXGIAdapter4> candidate;
		hr = factory_->EnumAdapterByGpuPreference(
			i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&candidate));
		if (hr == DXGI_ERROR_NOT_FOUND) { break; }
		if (FAILED(hr)) { continue; }

		DXGI_ADAPTER_DESC3 desc{};
		candidate->GetDesc3(&desc);

		// Microsoft WARP はスキップ
		if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) { continue; }

		// D3D12 デバイスを作れるか試す
		hr = D3D12CreateDevice(candidate.Get(), kMinFeatureLevel, __uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr)) {
			adapter_ = candidate;
			return true;
		}
	}

	// 列挙順に採用
	for (UINT i = 0;; i++) {
		ComPtr<IDXGIAdapter1> candidate;
		hr = factory_->EnumAdapters1(i, &candidate);
		if (hr == DXGI_ERROR_NOT_FOUND) { break; }

		DXGI_ADAPTER_DESC1 desc{};
		candidate->GetDesc1(&desc);

		// Microsoft WARP はスキップ
		if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) { continue; }

		// D3D12 デバイスを作れるか試す
		hr = D3D12CreateDevice(candidate.Get(), kMinFeatureLevel, __uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr)) {
			candidate.As(&adapter_);
			return true;
		}
	}

	// Microsoft WARP を採用する
	ComPtr<IDXGIAdapter1> warp;
	hr = factory_->EnumWarpAdapter(IID_PPV_ARGS(&warp));
	if (SUCCEEDED(hr)) {
		warp.As(&adapter_);
		return true;
	}

	// アダプタを採用できなかった
	return false;
}


bool ou::D3D12Device::CreateDevice() {
	auto hr = D3D12CreateDevice(adapter_.Get(), kMinFeatureLevel, IID_PPV_ARGS(&device_));
	if (SUCCEEDED(hr)) {
		device_->SetName(L"D3D12 Device");
		return true;
	}

	return false;
}


void ou::D3D12Device::SetUpInfoQueue() {
	if (!desc_.enableDebugLayer) { return; }

	ComPtr<ID3D12InfoQueue> infoQueue;
	auto hr = device_.As(&infoQueue);
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
	D3D12_INFO_QUEUE_FILTER filter{};
	filter.DenyList.NumIDs = _countof(denyIds);
	filter.DenyList.pIDList = denyIds;
	filter.DenyList.NumSeverities = _countof(denySeverities);
	filter.DenyList.pSeverityList = denySeverities;
	infoQueue->PushStorageFilter(&filter);

	// Windos11 以降なら、メッセージをログに流す
	ComPtr<ID3D12InfoQueue1> infoQueue1;
	hr = device_.As(&infoQueue1);
	if(SUCCEEDED(hr)) {
		infoQueue1->RegisterMessageCallback(OnD3D12Message, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &messageCallbackCockie_);
	}
}
