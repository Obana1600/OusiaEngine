#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>
#include <wrl/client.h>


namespace ou {

// デバイスの設定
// TODO: useWarp とか minFeatureLevel を入れてもいいかも
struct D3D12DeviceDesc {
	bool enableDebugLayer = false;
	bool enableGpuBasedValidation = false;
	bool enableDred = false;
};


class D3D12Device {
public: // 静的ファクトリ

	static std::unique_ptr<D3D12Device> Create(const D3D12DeviceDesc &desc);


public: // メンバ関数

	~D3D12Device();

	HRESULT GetDeviceRemovedReason() const;
	void LogDeviceRemovedInfo() const;

	IDXGIFactory6 *Factory() const;
	IDXGIAdapter4 *Adapter() const;
	ID3D12Device *Device() const;


private: // メンバ変数

	Microsoft::WRL::ComPtr<IDXGIFactory6> factory_;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;

	D3D12DeviceDesc desc_{};

	// DirectX のメッセージをログに流すコールバックの登録番号
	DWORD messageCallbackCockie_ = 0;


private: // メンバ関数
	bool EnableDebugLayer();
	bool EnableDred();
	bool CreateFactory();
	bool SelectAdapter();
	bool CreateDevice();
	void SetUpInfoQueue();


private: // コンストラクタ

	D3D12Device() = default;

	D3D12Device(const D3D12Device &) = delete;
	D3D12Device &operator=(const D3D12Device &) = delete;
	D3D12Device(D3D12Device &&) = delete;
	D3D12Device &operator=(D3D12Device &&) = delete;
};

} // namespace ou
