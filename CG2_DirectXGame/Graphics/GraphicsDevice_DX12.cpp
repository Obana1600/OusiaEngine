#include "GraphicsDevice_DX12.h"
#include "Core/Assert.h"
#include "Core/Fatal.h"
#include "Core/Log.h"
#include "Core/Paths.h"

#include <bit>
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

// バインドレスヒープに置ける SRV / UAV / CBV の数
constexpr UINT BINDLESS_RESOURCE_CAPACITY = 100'000;

// 固定ルートシグネチャのパラメータ番号
enum RootParameterIndex : UINT {
	ROOT_CBV_BEGIN = 0,
	ROOT_PARAMETER_COUNT = ROOT_CBV_BEGIN + DESCRIPTORBINDER_CBV_COUNT,
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

void CheckRequiredFeatures(ID3D12Device *device) {
	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel{D3D_SHADER_MODEL_6_6};
	HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
	if (FAILED(hr) || shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_6) {
		FatalError("この GPU は Shader Model 6.6 に対応していません。");
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
	hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
	if (FAILED(hr) || options.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_3) {
		FatalError("この GPU は Resource Binding Tier 3 に対応していません。");
	}
}

D3D12_STATIC_SAMPLER_DESC MakeStaticSampler(
	UINT shaderRegister, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE address) {
	D3D12_STATIC_SAMPLER_DESC desc{};
	desc.Filter = filter;
	desc.AddressU = address;
	desc.AddressV = address;
	desc.AddressW = address;
	desc.MipLODBias = 0.0f;
	desc.MaxAnisotropy = 0;
	desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	desc.MinLOD = 0;
	desc.MaxLOD = D3D12_FLOAT32_MAX;
	desc.ShaderRegister = shaderRegister;
	desc.RegisterSpace = 0;
	desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	return desc;
}

ComPtr<ID3D12RootSignature> CreateGlobalRootSignature(ID3D12Device *device) {
	// b0 ~ b3: ルート CBV
	D3D12_ROOT_PARAMETER1 params[ROOT_PARAMETER_COUNT]{};
	for (UINT i = 0; i < DESCRIPTORBINDER_CBV_COUNT; i++) {
		auto &param = params[ROOT_CBV_BEGIN + i];
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		param.Descriptor.ShaderRegister = i;
		param.Descriptor.RegisterSpace = 0;
		param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	// s0 ~ s3: サンプラー
	const D3D12_STATIC_SAMPLER_DESC staticSamplers[] = {
		MakeStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP),
		MakeStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP),
		MakeStaticSampler(2, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP),
		MakeStaticSampler(3, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP),
	};

	// ルートシグネチャ
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc{};
	desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	desc.Desc_1_1.NumParameters = ROOT_PARAMETER_COUNT;
	desc.Desc_1_1.pParameters = params;
	desc.Desc_1_1.NumStaticSamplers = _countof(staticSamplers);
	desc.Desc_1_1.pStaticSamplers = staticSamplers;
	desc.Desc_1_1.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> error;
	HRESULT hr = D3D12SerializeVersionedRootSignature(&desc, &blob, &error);
	if (FAILED(hr)) {
		if (error) {
			Log::Error("{}", static_cast<const char *>(error->GetBufferPointer()));
		}
		FatalError("ルートシグネチャのシリアライズに失敗しました。");
	}

	ComPtr<ID3D12RootSignature> rootSignature;
	hr = device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	CheckHResult(hr, "ルートシグネチャの作成");
	rootSignature->SetName(L"Global RootSignature");

	return rootSignature;
}

} // namespace



namespace ou::Internal {

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

constexpr D3D12_FILL_MODE ConvertFillMode(FillMode value) {
	switch (value) {
		case FillMode::WIREFRAME:
			return D3D12_FILL_MODE_WIREFRAME;
		case FillMode::SOLID:
			return D3D12_FILL_MODE_SOLID;
	}
	return D3D12_FILL_MODE_SOLID;
}

constexpr D3D12_CULL_MODE ConvertCullMode(CullMode value) {
	switch (value) {
		case CullMode::NONE:
			return D3D12_CULL_MODE_NONE;
		case CullMode::FRONT:
			return D3D12_CULL_MODE_FRONT;
		case CullMode::BACK:
			return D3D12_CULL_MODE_BACK;
	}
	return D3D12_CULL_MODE_NONE;
}

constexpr D3D12_COMPARISON_FUNC ConvertComparisonFunc(ComparisonFunc value) {
	switch (value) {
		case ComparisonFunc::NEVER:
			return D3D12_COMPARISON_FUNC_NEVER;
		case ComparisonFunc::LESS:
			return D3D12_COMPARISON_FUNC_LESS;
		case ComparisonFunc::EQUAL:
			return D3D12_COMPARISON_FUNC_EQUAL;
		case ComparisonFunc::LESS_EQUAL:
			return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case ComparisonFunc::GREATER:
			return D3D12_COMPARISON_FUNC_GREATER;
		case ComparisonFunc::NOT_EQUAL:
			return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case ComparisonFunc::GREATER_EQUAL:
			return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case ComparisonFunc::ALWAYS:
			return D3D12_COMPARISON_FUNC_ALWAYS;
	}
	return D3D12_COMPARISON_FUNC_NEVER;
}

constexpr D3D12_BLEND ConvertBlend(Blend value) {
	switch (value) {
		case Blend::ZERO:
			return D3D12_BLEND_ZERO;
		case Blend::ONE:
			return D3D12_BLEND_ONE;
		case Blend::SRC_COLOR:
			return D3D12_BLEND_SRC_COLOR;
		case Blend::INV_SRC_COLOR:
			return D3D12_BLEND_INV_SRC_COLOR;
		case Blend::SRC_ALPHA:
			return D3D12_BLEND_SRC_ALPHA;
		case Blend::INV_SRC_ALPHA:
			return D3D12_BLEND_INV_SRC_ALPHA;
		case Blend::DEST_COLOR:
			return D3D12_BLEND_DEST_COLOR;
		case Blend::INV_DEST_COLOR:
			return D3D12_BLEND_INV_DEST_COLOR;
		case Blend::DEST_ALPHA:
			return D3D12_BLEND_DEST_ALPHA;
		case Blend::INV_DEST_ALPHA:
			return D3D12_BLEND_INV_DEST_ALPHA;
	}
	return D3D12_BLEND_ZERO;
}

constexpr D3D12_BLEND_OP ConvertBlendOp(BlendOp value) {
	switch (value) {
		case BlendOp::ADD:
			return D3D12_BLEND_OP_ADD;
		case BlendOp::SUBTRACT:
			return D3D12_BLEND_OP_SUBTRACT;
		case BlendOp::REV_SUBTRACT:
			return D3D12_BLEND_OP_REV_SUBTRACT;
		case BlendOp::MIN:
			return D3D12_BLEND_OP_MIN;
		case BlendOp::MAX:
			return D3D12_BLEND_OP_MAX;
	}
	return D3D12_BLEND_OP_ADD;
}

constexpr D3D12_INPUT_CLASSIFICATION ConvertInputClassification(InputClassification value) {
	switch (value) {
		case InputClassification::PER_VERTEX_DATA:
			return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		case InputClassification::PER_INSTANCE_DATA:
			return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
	}
	return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
}

constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertTopologyType(PrimitiveTopology value) {
	switch (value) {
		case PrimitiveTopology::UNDEFINED:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		case PrimitiveTopology::TRIANGLELIST:
		case PrimitiveTopology::TRIANGLESTRIP:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		case PrimitiveTopology::POINTLIST:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		case PrimitiveTopology::LINELIST:
		case PrimitiveTopology::LINESTRIP:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		case PrimitiveTopology::PATCHLIST:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	}
	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

constexpr D3D_PRIMITIVE_TOPOLOGY ConvertTopology(PrimitiveTopology value, uint32_t patchControlPoints) {
	switch (value) {
		case PrimitiveTopology::TRIANGLELIST:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case PrimitiveTopology::TRIANGLESTRIP:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case PrimitiveTopology::POINTLIST:
			return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case PrimitiveTopology::LINELIST:
			return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case PrimitiveTopology::LINESTRIP:
			return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case PrimitiveTopology::PATCHLIST:
			return static_cast<D3D_PRIMITIVE_TOPOLOGY>(
				D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (patchControlPoints - 1));
		default:
			break;
	}
	return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

constexpr D3D12_RESOURCE_STATES ConvertResourceState(ResourceState value) {
	auto result = D3D12_RESOURCE_STATE_COMMON;

	if (HasFlag(value, ResourceState::SHADER_RESOURCE)) {
		result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	}
	if (HasFlag(value, ResourceState::SHADER_RESOURCE_COMPUTE)) {
		result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	}
	if (HasFlag(value, ResourceState::UNORDERED_ACCESS)) {
		result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
	if (HasFlag(value, ResourceState::COPY_SRC)) {
		result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
	}
	if (HasFlag(value, ResourceState::COPY_DST)) {
		result |= D3D12_RESOURCE_STATE_COPY_DEST;
	}
	if (HasFlag(value, ResourceState::RENDERTARGET)) {
		result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
	}
	if (HasFlag(value, ResourceState::DEPTHSTENCIL)) {
		result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}
	if (HasFlag(value, ResourceState::DEPTHSTENCIL_READONLY)) {
		result |= D3D12_RESOURCE_STATE_DEPTH_READ;
	}
	if (HasFlag(value, ResourceState::VERTEX_BUFFER) || HasFlag(value, ResourceState::CONSTANT_BUFFER)) {
		result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	}
	if (HasFlag(value, ResourceState::INDEX_BUFFER)) {
		result |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
	}
	if (HasFlag(value, ResourceState::INDIRECT_ARGUMENT)) {
		result |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
	}

	return result;
}


// スワップチェーン ==================================================================================

struct SwapChain_DX12 {
	std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationHandler;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> backBuffers;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> backBufferRtvs;

	~SwapChain_DX12();
};

SwapChain_DX12::~SwapChain_DX12() {
	if (!allocationHandler) { return; }

	// RTV はすぐ返してよい
	for (const auto &rtv : backBufferRtvs) {
		allocationHandler->descriptorRtv.Free(rtv);
	}

	// バックバッファは GPU が使い終わるまで解放を遅らせる
	std::scoped_lock lock(allocationHandler->destroyLocker);
	const auto frameCount = allocationHandler->frameCount;
	for (auto &backBuffer : backBuffers) {
		allocationHandler->destroyerResources.push_back({backBuffer, frameCount});
	}
}

SwapChain_DX12 *ToInternal(const SwapChain *param) {
	return static_cast<SwapChain_DX12 *>(param->internalState.get());
}

// ================================================================================================


// リソース =========================================================================================

struct Resource_DX12 {
	std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationHandler;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;

	int srvIndex = -1;

	virtual ~Resource_DX12();
};

Resource_DX12::~Resource_DX12() {
	if (!allocationHandler) { return; }

	// GPU が使い終わったら、リソースとバインドレス番号を手放す
	std::scoped_lock lock(allocationHandler->destroyLocker);
	const auto frameCount = allocationHandler->frameCount;
	if (resource) {
		allocationHandler->destroyerResources.push_back({resource, frameCount});
	}
	if (srvIndex >= 0) {
		allocationHandler->destroyBindlessRes.push_back({srvIndex, frameCount});
	}
}

Resource_DX12 *ToInternal(const GpuResource *param) {
	return static_cast<Resource_DX12 *>(param->internalState.get());
}

// ================================================================================================


// テクスチャ =======================================================================================

struct Texture_DX12: public Resource_DX12 {
	D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
	D3D12_CPU_DESCRIPTOR_HANDLE dsv{};

	~Texture_DX12();
};


Texture_DX12::~Texture_DX12() {
	if (!allocationHandler) { return; }

	// RTV / DSV はすぐ返してよい
	if (rtv.ptr != 0) { allocationHandler->descriptorRtv.Free(rtv); }
	if (dsv.ptr != 0) { allocationHandler->descriptorDsv.Free(dsv); }
}

Texture_DX12 *ToInternal(const Texture *param) {
	return static_cast<Texture_DX12 *>(param->internalState.get());
}

// ================================================================================================


// シェーダ =========================================================================================

struct Shader_DX12 {
	std::vector<uint8_t> shaderCode;

	D3D12_SHADER_BYTECODE GetByteCode() const {
		return {shaderCode.data(), shaderCode.size()};
	}
};

Shader_DX12 *ToInternal(const Shader *param) {
	return static_cast<Shader_DX12 *>(param->internalState.get());
}

// ================================================================================================


// パイプライン =====================================================================================

struct PipelineState_DX12 {
	std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationHandler;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	D3D_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

	~PipelineState_DX12();
};

PipelineState_DX12::~PipelineState_DX12() {
	if (!allocationHandler) { return; }

	std::scoped_lock lock(allocationHandler->destroyLocker);
	if (pipelineState) {
		allocationHandler->destroyerPipelines.push_back({pipelineState, allocationHandler->frameCount});
	}
}

PipelineState_DX12 *ToInternal(const PipelineState *param) {
	return static_cast<PipelineState_DX12 *>(param->internalState.get());
}

// ================================================================================================

} // namespace ou::Internal

using namespace ou::Internal;



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

	CheckRequiredFeatures(device_.Get());

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
	allocationHandler_->descriptorRtv.Initialize(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256);
	allocationHandler_->descriptorDsv.Initialize(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 64);

	// ----- 初期データの転送 -----
	copyAllocator_.Initialize(this);

	// ----- ルートシグネチャ -----
	rootSignature_ = CreateGlobalRootSignature(device_.Get());

	// ----- バインドレスヒープ -----
	D3D12_DESCRIPTOR_HEAP_DESC resHeapDesc{};
	resHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	resHeapDesc.NumDescriptors = BINDLESS_RESOURCE_CAPACITY;
	resHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	hr = device_->CreateDescriptorHeap(&resHeapDesc, IID_PPV_ARGS(&descriptorHeapRes_));
	CheckHResult(hr, "バインドレスヒープの作成");
	descriptorHeapRes_->SetName(L"BindlessResourceHeap");

	// バインドレス関連
	resDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	allocationHandler_->freeBindlessRes.reserve(BINDLESS_RESOURCE_CAPACITY);
	for (int i = static_cast<int>(BINDLESS_RESOURCE_CAPACITY) - 1; i >= 0; i--) {
		allocationHandler_->freeBindlessRes.push_back(i);
	}

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

	auto is = std::static_pointer_cast<SwapChain_DX12>(swapChain->internalState);
	if (!is) {
		is = std::make_shared<SwapChain_DX12>();
	}
	is->allocationHandler = allocationHandler_;
	swapChain->internalState = is;
	swapChain->desc = *desc;

	UINT flags = 0;
	if (tearingSupported_) { flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; }

	const auto rtvFormat = ConvertFormat(desc->format);
	const auto bufferFormat = ConvertFormatNonSrgb(rtvFormat);

	// 新しく作る
	if (!is->swapChain) {
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

		hr = swapChain1.As(&is->swapChain);
		if (FAILED(hr)) {
			Log::Error("IDXGISwapChain4 の取得に失敗しました。");
			return false;
		}

	} else {
		// リサイズ
		WaitForGpu();
		for (const auto &rtv : is->backBufferRtvs) {
			allocationHandler_->descriptorRtv.Free(rtv);
		}
		is->backBuffers.clear();
		is->backBufferRtvs.clear();

		hr = is->swapChain->ResizeBuffers(
			desc->bufferCount, desc->width, desc->height, bufferFormat, flags);
		if (FAILED(hr)) {
			Log::Error("スワップチェーンのリサイズに失敗しました。");
			return false;
		}
	}

	// バックバッファの RTV を借りてくる
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = rtvFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	is->backBuffers.resize(desc->bufferCount);
	is->backBufferRtvs.resize(desc->bufferCount);
	for (uint32_t i = 0; i < desc->bufferCount; i++) {
		hr = is->swapChain->GetBuffer(i, IID_PPV_ARGS(&is->backBuffers[i]));
		if (FAILED(hr)) {
			Log::Error("バックバッファの取得に失敗しました。");
			return false;
		}

		is->backBufferRtvs[i] = allocationHandler_->descriptorRtv.Allocate();
		device_->CreateRenderTargetView(is->backBuffers[i].Get(), &rtvDesc, is->backBufferRtvs[i]);
	}

	return true;
}


bool ou::GraphicsDevice_DX12::CreateBuffer(const GpuBufferDesc *desc, const void *initialData, GpuBuffer *buffer) {
	HRESULT hr{};

	auto is = std::make_shared<Resource_DX12>();
	is->allocationHandler = allocationHandler_;
	buffer->internalState = is;
	buffer->type = GpuResource::Type::BUFFER;
	buffer->mappedData = nullptr;
	buffer->mappedSize = 0;
	buffer->desc = *desc;

	// 定数バッファを 256 バイトに切り上げる
	auto alignedSize = desc->size;
	if (HasFlag(desc->bindFlags, BindFlag::CONSTANT_BUFFER)) {
		alignedSize = AlignTo(alignedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	// リソースの設定
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Alignment = 0;
	resDesc.Width = alignedSize;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc = {1, 0};
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	if (HasFlag(desc->bindFlags, BindFlag::UNORDERED_ACCESS)) {
		resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	// ヒープの種類と初期化
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	auto initialState = D3D12_RESOURCE_STATE_COMMON;

	switch (desc->heapType) {
		case HeapType::UPLOAD:
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
			break;

		case HeapType::READBACK:
			heapProps.Type = D3D12_HEAP_TYPE_READBACK;
			initialState = D3D12_RESOURCE_STATE_COPY_DEST;
			resDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
			break;

		default:
			break;
	}

	hr = device_->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, initialState, nullptr, IID_PPV_ARGS(&is->resource));
	if (FAILED(hr)) {
		Log::Error("バッファの作成に失敗しました。");
		return false;
	}

	is->gpuAddress = is->resource->GetGPUVirtualAddress();

	// マッピング
	if (desc->heapType == HeapType::UPLOAD) {
		D3D12_RANGE readRange{0, 0};
		hr = is->resource->Map(0, &readRange, &buffer->mappedData);
		CheckHResult(hr, "アップロードバッファの Map");
		buffer->mappedSize = static_cast<size_t>(alignedSize);

	} else if (desc->heapType == HeapType::READBACK) {
		hr = is->resource->Map(0, nullptr, &buffer->mappedData);
		CheckHResult(hr, "リードバックバッファの Map");
		buffer->mappedSize = static_cast<size_t>(alignedSize);
	}

	// 初期データ
	if (initialData) {
		if (desc->heapType == HeapType::UPLOAD) {
			std::memcpy(buffer->mappedData, initialData, desc->size);

		} else if (desc->heapType == HeapType::DEFAULT) {
			auto cmd = copyAllocator_.Allocate(desc->size);
			std::memcpy(cmd.uploadBuffer.mappedData, initialData, desc->size);

			cmd.list->CopyBufferRegion(
				is->resource.Get(), 0, ToInternal(&cmd.uploadBuffer)->resource.Get(), 0, desc->size);

			copyAllocator_.Submit(std::move(cmd));
		}
	}

	return true;
}


bool ou::GraphicsDevice_DX12::CreateTexture(const TextureDesc *desc, const SubresourceData *initialData, Texture *texture) {
	HRESULT hr{};

	if (HasFlag(desc->bindFlags, BindFlag::DEPTH_STENCIL) && HasFlag(desc->bindFlags, BindFlag::SHADER_RESOURCE)) {
		Log::Error("深度テクスチャの SRV にはまだ対応していません。");
		return false;
	}

	auto is = std::make_shared<Texture_DX12>();
	is->allocationHandler = allocationHandler_;
	texture->internalState = is;
	texture->type = GpuResource::Type::TEXTURE;
	texture->mappedData = nullptr;
	texture->mappedSize = 0;
	texture->desc = *desc;

	// リソースの設定
	D3D12_RESOURCE_DESC resDesc{};
	switch (desc->type) {
		case TextureDesc::Type::TEXTURE_1D:
			resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			break;
		case TextureDesc::Type::TEXTURE_2D:
			resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			break;
		case TextureDesc::Type::TEXTURE_3D:
			resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			break;
	}
	resDesc.Alignment = 0;
	resDesc.Width = desc->width;
	resDesc.Height = desc->height;
	resDesc.DepthOrArraySize = static_cast<UINT16>(
		desc->type == TextureDesc::Type::TEXTURE_3D ? desc->depth : desc->arraySize);
	resDesc.MipLevels = static_cast<UINT16>(desc->mipLevels);
	resDesc.Format = ConvertFormat(desc->format);
	resDesc.SampleDesc = {desc->sampleCount, 0};
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // GPU に任せる
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	if (HasFlag(desc->bindFlags, BindFlag::RENDER_TARGET)) {
		resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	if (HasFlag(desc->bindFlags, BindFlag::DEPTH_STENCIL)) {
		resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		if (!HasFlag(desc->bindFlags, BindFlag::SHADER_RESOURCE)) {
			resDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}
	}
	if (HasFlag(desc->bindFlags, BindFlag::UNORDERED_ACCESS)) {
		resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	// クリア値
	D3D12_CLEAR_VALUE optimizedClear{};
	optimizedClear.Format = resDesc.Format;
	const D3D12_CLEAR_VALUE *pOptimizedClear = nullptr;
	if (HasFlag(desc->bindFlags, BindFlag::RENDER_TARGET)) {
		std::memcpy(optimizedClear.Color, desc->clear.color, sizeof(optimizedClear.Color));
		pOptimizedClear = &optimizedClear;

	} else if (HasFlag(desc->bindFlags, BindFlag::DEPTH_STENCIL)) {
		optimizedClear.DepthStencil.Depth = desc->clear.depthStencil.depth;
		optimizedClear.DepthStencil.Stencil = static_cast<UINT8>(desc->clear.depthStencil.stencil);
		pOptimizedClear = &optimizedClear;
	}

	// 初期状態
	const auto layoutState = ConvertResourceState(desc->layout);
	const auto initialState = initialData ? D3D12_RESOURCE_STATE_COMMON : layoutState;

	// テクスチャの作成
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	hr = device_->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, initialState, pOptimizedClear, IID_PPV_ARGS(&is->resource));
	if (FAILED(hr)) {
		Log::Error("テクスチャの作成に失敗しました。");
		return false;
	}

	// 初期データの転送
	if (initialData) {
		const UINT numSubresources = (desc->type == TextureDesc::Type::TEXTURE_3D)
										 ? desc->mipLevels
										 : desc->mipLevels * desc->arraySize;

		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(numSubresources);
		std::vector<UINT> numRows(numSubresources);
		std::vector<UINT64> rowSizes(numSubresources);
		UINT64 totalBytes = 0;
		device_->GetCopyableFootprints(
			&resDesc, 0, numSubresources, 0, layouts.data(), numRows.data(), rowSizes.data(), &totalBytes);

		auto cmd = copyAllocator_.Allocate(totalBytes);
		auto *stagingBase = static_cast<uint8_t *>(cmd.uploadBuffer.mappedData);
		auto *stagingResource = ToInternal(&cmd.uploadBuffer)->resource.Get();

		for (UINT i = 0; i < numSubresources; i++) {
			const auto &layout = layouts[i];
			const auto &src = initialData[i];

			for (UINT z = 0; z < layout.Footprint.Depth; z++) {
				for (UINT row = 0; row < numRows[i]; row++) {
					auto *dst = stagingBase + layout.Offset +
								static_cast<SIZE_T>(z) * layout.Footprint.RowPitch * numRows[i] +
								static_cast<SIZE_T>(row) * layout.Footprint.RowPitch;
					const auto *srcRow = static_cast<const uint8_t *>(src.dataPtr) +
										 static_cast<SIZE_T>(z) * src.slicePitch +
										 static_cast<SIZE_T>(row) * src.rowPitch;
					std::memcpy(dst, srcRow, static_cast<size_t>(rowSizes[i]));
				}
			}

			D3D12_TEXTURE_COPY_LOCATION dstLocation{};
			dstLocation.pResource = is->resource.Get();
			dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dstLocation.SubresourceIndex = i;

			D3D12_TEXTURE_COPY_LOCATION srcLocation{};
			srcLocation.pResource = stagingResource;
			srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			srcLocation.PlacedFootprint = layout;

			cmd.list->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
		}

		copyAllocator_.Submit(std::move(cmd));

		// コピーを終えた後の遷移を予約する
		if (layoutState != D3D12_RESOURCE_STATE_COMMON) {
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = is->resource.Get();
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
			barrier.Transition.StateAfter = layoutState;

			std::scoped_lock lock(initTransitionLocker_);
			initTransitions_.push_back(barrier);
		}
	}

	// SRV
	if (HasFlag(desc->bindFlags, BindFlag::SHADER_RESOURCE)) {
		is->srvIndex = allocationHandler_->AllocateResourceIndex();
		if (is->srvIndex < 0) {
			FatalError("バインドレスヒープの空きがありません。");
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = resDesc.Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		if (desc->type == TextureDesc::Type::TEXTURE_2D && desc->arraySize == 1) {
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = desc->mipLevels;

		} else {
			// TODO: 1D / 3D / 配列 / キューブ
			Log::Error("この種類のテクスチャの SRV にはまだ対応していません。");
			return false;
		}

		device_->CreateShaderResourceView(is->resource.Get(), &srvDesc, GetBindlessCpuHandle(is->srvIndex));
	}

	// RTV / DSV
	if (HasFlag(desc->bindFlags, BindFlag::RENDER_TARGET)) {
		is->rtv = allocationHandler_->descriptorRtv.Allocate();
		device_->CreateRenderTargetView(is->resource.Get(), nullptr, is->rtv);
	}
	if (HasFlag(desc->bindFlags, BindFlag::DEPTH_STENCIL)) {
		is->dsv = allocationHandler_->descriptorDsv.Allocate();
		device_->CreateDepthStencilView(is->resource.Get(), nullptr, is->dsv);
	}

	return true;
}


bool ou::GraphicsDevice_DX12::CreateShader(ShaderStage stage, const void *shaderCode, size_t shaderCodeSize, Shader *shader) {
	if (!shaderCode || shaderCodeSize == 0) {
		Log::Error("シェーダのバイトコードが空です。");
		return false;
	}

	auto is = std::make_shared<Shader_DX12>();
	shader->internalState = is;
	shader->stage = stage;

	is->shaderCode.resize(shaderCodeSize);
	std::memcpy(is->shaderCode.data(), shaderCode, shaderCodeSize);

	// TODO: CS に対応させる

	return true;
}


bool ou::GraphicsDevice_DX12::CreateSampler(const SamplerDesc *desc, Sampler *sampler) {
	(void)desc;
	(void)sampler;
	return false;
}


bool ou::GraphicsDevice_DX12::CreatePipelineState(const PipelineStateDesc *desc, PipelineState *pso, const RenderPassInfo *renderPassInfo) {
	if (!renderPassInfo) {
		// TODO: 描画時の PSO ソートに対応させる
		Log::Error("CreatePipelineState には RenderPassInfo が必要です。");
		return false;
	}
	if (desc->ms || desc->as) {
		// TODO: メッシュシェーダ PSO の対応
		Log::Error("メッシュシェーダの PSO にはまだ対応していません。");
		return false;
	}

	auto is = std::make_shared<PipelineState_DX12>();
	is->allocationHandler = allocationHandler_;
	pso->internalState = is;
	pso->desc = *desc;

	// PSO の設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature_.Get();

	// シェーダ
	if (desc->vs) { psoDesc.VS = ToInternal(desc->vs)->GetByteCode(); }
	if (desc->ps) { psoDesc.PS = ToInternal(desc->ps)->GetByteCode(); }
	if (desc->hs) { psoDesc.HS = ToInternal(desc->hs)->GetByteCode(); }
	if (desc->ds) { psoDesc.DS = ToInternal(desc->ds)->GetByteCode(); }
	if (desc->gs) { psoDesc.GS = ToInternal(desc->gs)->GetByteCode(); }

	// インプットレイアウト
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
	if (desc->il) {
		inputElements.reserve(desc->il->elements.size());
		for (const auto &element : desc->il->elements) {
			auto &e = inputElements.emplace_back();
			e.SemanticName = element.semanticName.c_str();
			e.SemanticIndex = element.semanticIndex;
			e.Format = ConvertFormat(element.format);
			e.InputSlot = element.inputSlot;
			e.AlignedByteOffset = element.alignedByteOffset;
			e.InputSlotClass = ConvertInputClassification(element.inputSlotClass);
			e.InstanceDataStepRate = (element.inputSlotClass == InputClassification::PER_INSTANCE_DATA) ? 1 : 0;
		}
	}
	psoDesc.InputLayout.pInputElementDescs = inputElements.data();
	psoDesc.InputLayout.NumElements = static_cast<UINT>(inputElements.size());

	// ラスタライザ
	const auto rs = desc->rs ? *desc->rs : RasterizerState{};
	psoDesc.RasterizerState.FillMode = ConvertFillMode(rs.fillMode);
	psoDesc.RasterizerState.CullMode = ConvertCullMode(rs.cullMode);
	psoDesc.RasterizerState.FrontCounterClockwise = rs.frontCounterClockwise;
	psoDesc.RasterizerState.DepthBias = rs.depthBias;
	psoDesc.RasterizerState.DepthBiasClamp = rs.depthBiasClamp;
	psoDesc.RasterizerState.SlopeScaledDepthBias = rs.slopeScaledDepthBias;
	psoDesc.RasterizerState.DepthClipEnable = rs.depthClipEnable;
	psoDesc.RasterizerState.MultisampleEnable = rs.multisampleEnable;
	psoDesc.RasterizerState.AntialiasedLineEnable = rs.antialiasedLineEnable;
	psoDesc.RasterizerState.ConservativeRaster = rs.conservativeRasterizationEnable
													 ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON
													 : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// 深度ステンシル
	const auto dss = desc->dss ? *desc->dss : DepthStencilState{};
	psoDesc.DepthStencilState.DepthEnable = dss.depthEnable;
	psoDesc.DepthStencilState.DepthWriteMask = (dss.depthWriteMask == DepthWriteMask::ALL)
												   ? D3D12_DEPTH_WRITE_MASK_ALL
												   : D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = ConvertComparisonFunc(dss.comparisonFunc);
	psoDesc.DepthStencilState.StencilEnable = FALSE; // TODO: ステンシルに対応させる

	// ブレンド
	const auto bs = desc->bs ? *desc->bs : BlendState{};
	psoDesc.BlendState.AlphaToCoverageEnable = bs.alphaToCoverageEnable;
	psoDesc.BlendState.IndependentBlendEnable = bs.independentBlendEnable;
	for (int i = 0; i < 8; i++) {
		const auto &src = bs.renderTarget[i];
		auto &dst = psoDesc.BlendState.RenderTarget[i];
		dst.BlendEnable = src.blendEnable;
		dst.LogicOpEnable = FALSE;
		dst.SrcBlend = ConvertBlend(src.srcBlend);
		dst.SrcBlendAlpha = ConvertBlend(src.srcBlendAlpha);
		dst.DestBlend = ConvertBlend(src.destBlend);
		dst.DestBlendAlpha = ConvertBlend(src.destBlendAlpha);
		dst.BlendOp = ConvertBlendOp(src.blendOp);
		dst.BlendOpAlpha = ConvertBlendOp(src.blendOpAlpha);
		dst.LogicOp = D3D12_LOGIC_OP_NOOP;
		dst.RenderTargetWriteMask = static_cast<UINT8>(src.renderTargetWriteMask);
	}
	psoDesc.SampleMask = desc->sampleMask;

	// トポロジ
	psoDesc.PrimitiveTopologyType = ConvertTopologyType(desc->pt);
	is->primitiveTopology = ConvertTopology(desc->pt, desc->patchControlPoints);

	// 出力先
	psoDesc.NumRenderTargets = renderPassInfo->rtCount;
	for (uint32_t i = 0; i < renderPassInfo->rtCount; i++) {
		psoDesc.RTVFormats[i] = ConvertFormat(renderPassInfo->rtFormats[i]);
	}
	psoDesc.DSVFormat = ConvertFormat(renderPassInfo->dsFormat);
	psoDesc.SampleDesc.Count = renderPassInfo->sampleCount;
	psoDesc.SampleDesc.Quality = 0;

	// PSO の作成
	HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&is->pipelineState));
	if (FAILED(hr)) {
		Log::Error("PipelineState の作成に失敗しました");
		return false;
	}

	return true;
}


void ou::GraphicsDevice_DX12::SetName(GpuResource *resource, std::string_view name) const {
	if (!resource || !resource->IsValid()) { return; }

	auto *is = ToInternal(resource);
	if (is->resource) {
		is->resource->SetName(Utf8ToWide(name).c_str());
	}
}


int ou::GraphicsDevice_DX12::GetDescriptorIndex(const GpuResource *resource, ViewType type, int subresource) const {
	(void)subresource; // TODO: ミップやスライスごとのビュー

	if (!resource || !resource->IsValid()) { return -1; }

	// TODO: UAV
	const auto *is = ToInternal(resource);
	switch (type) {
		case ViewType::SRV:
			return is->srvIndex;
		default:
			return -1;
	}

	return 0;
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

	// バインドレスヒープとルートシグネチャをセットしておく
	if (queue == QUEUE_GRAPHICS) {
		auto *list = cmdDX12.GetGraphicsCommandList();

		ID3D12DescriptorHeap *heaps[] = {descriptorHeapRes_.Get()};
		list->SetDescriptorHeaps(_countof(heaps), heaps);
		list->SetGraphicsRootSignature(rootSignature_.Get());

		// Texture をまとめて遷移させる
		std::scoped_lock lock(initTransitionLocker_);
		if (!initTransitions_.empty()) {
			list->ResourceBarrier(static_cast<UINT>(initTransitions_.size()), initTransitions_.data());
			initTransitions_.clear();
		}
	}

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
	auto &cmdDX12 = GetCommandList(cmd);
	auto *list = cmdDX12.GetGraphicsCommandList();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[8]{};
	UINT rtCount = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
	bool hasDsv = false;

	std::vector<D3D12_RESOURCE_BARRIER> beginBarriers;

	// バリアの準備と、 RTV / DSV の収集
	for (uint32_t i = 0; i < imageCount; i++) {
		const auto &image = images[i];
		auto *is = ToInternal(image.texture);

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = is->resource.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES; // TODO: image.subresources

		// before -> layout
		const auto before = ConvertResourceState(image.layoutBefore);
		const auto during = ConvertResourceState(image.layout);
		const auto after = ConvertResourceState(image.layoutAfter);
		if (before != during) {
			barrier.Transition.StateBefore = before;
			barrier.Transition.StateAfter = during;
			beginBarriers.push_back(barrier);
		}
		// layout -> after
		if (during != after) {
			barrier.Transition.StateBefore = during;
			barrier.Transition.StateAfter = after;
			cmdDX12.renderPassBarriersEnd.push_back(barrier);
		}

		switch (image.type) {
			case RenderPassImage::Type::RENDERTARGET:
				rtvs[rtCount++] = is->rtv;
				break;
			case RenderPassImage::Type::DEPTH_STENCIL:
				dsv = is->dsv;
				hasDsv = true;
				break;
		}
	}

	if (!beginBarriers.empty()) {
		list->ResourceBarrier(static_cast<UINT>(beginBarriers.size()), beginBarriers.data());
	}

	list->OMSetRenderTargets(rtCount, rtvs, FALSE, hasDsv ? &dsv : nullptr);

	// LoadOp::CLEAR をクリアする
	for (uint32_t i = 0; i < imageCount; i++) {
		const auto &image = images[i];
		if (image.loadOp != RenderPassImage::LoadOp::CLEAR) { continue; }

		const auto &texDesc = image.texture->desc;
		auto *is = ToInternal(image.texture);

		switch (image.type) {
			case RenderPassImage::Type::RENDERTARGET:
				list->ClearRenderTargetView(is->rtv, texDesc.clear.color, 0, nullptr);
				break;

			case RenderPassImage::Type::DEPTH_STENCIL:
				{
					auto flags = D3D12_CLEAR_FLAG_DEPTH;
					if (texDesc.format == Format::D24_UNORM_S8_UINT) {
						flags |= D3D12_CLEAR_FLAG_STENCIL;
					}
					list->ClearDepthStencilView(
						is->dsv, flags, texDesc.clear.depthStencil.depth,
						static_cast<UINT8>(texDesc.clear.depthStencil.stencil), 0, nullptr);
					break;
				}
		}
	}
}


void ou::GraphicsDevice_DX12::RenderPassEnd(CommandList cmd) {
	auto &cmdDX12 = GetCommandList(cmd);
	auto &barriers = cmdDX12.renderPassBarriersEnd;

	if (!barriers.empty()) {
		cmdDX12.GetGraphicsCommandList()->ResourceBarrier(
			static_cast<UINT>(barriers.size()), barriers.data());
		barriers.clear();
	}
}


void ou::GraphicsDevice_DX12::BindViewports(uint32_t numViewports, const Viewport *viewports, CommandList cmd) {
	ASSERT(numViewports <= D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "D3D12 のビューポート上限を超過しています。");

	D3D12_VIEWPORT d3dViewports[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	for (uint32_t i = 0; i < numViewports; i++) {
		d3dViewports[i].TopLeftX = viewports[i].topLeftX;
		d3dViewports[i].TopLeftY = viewports[i].topLeftY;
		d3dViewports[i].Width = viewports[i].width;
		d3dViewports[i].Height = viewports[i].height;
		d3dViewports[i].MinDepth = viewports[i].minDepth;
		d3dViewports[i].MaxDepth = viewports[i].maxDepth;
	}

	GetCommandList(cmd).GetGraphicsCommandList()->RSSetViewports(numViewports, d3dViewports);
}


void ou::GraphicsDevice_DX12::BindScissorRects(uint32_t numScissors, const Scissor *scissors, CommandList cmd) {
	ASSERT(numScissors <= D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, " D3D12 のシザー矩形上限を超過しています: {}", numScissors);

	D3D12_RECT d3dScissors[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	for (uint32_t i = 0; i < numScissors; i++) {
		d3dScissors[i].left = scissors[i].left;
		d3dScissors[i].top = scissors[i].top;
		d3dScissors[i].right = scissors[i].right;
		d3dScissors[i].bottom = scissors[i].bottom;
	}

	GetCommandList(cmd).GetGraphicsCommandList()->RSSetScissorRects(numScissors, d3dScissors);
}


void ou::GraphicsDevice_DX12::BindPipelineState(const PipelineState *pso, CommandList cmd) {
	auto &cmdIs = GetCommandList(cmd);

	// 同じ PSO なら積まない
	if (cmdIs.activePso == pso) { return; }
	cmdIs.activePso = pso;

	auto *is = ToInternal(pso);
	auto *list = cmdIs.GetGraphicsCommandList();
	list->SetPipelineState(is->pipelineState.Get());
	list->IASetPrimitiveTopology(is->primitiveTopology);
}


void ou::GraphicsDevice_DX12::BindVertexBuffers(const GpuBuffer *const *buffers, uint32_t slot, uint32_t count, const uint32_t *strides, const uint64_t *offsets, CommandList cmd) {
	ASSERT(count <= D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);

	D3D12_VERTEX_BUFFER_VIEW views[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT]{};
	for (uint32_t i = 0; i < count; i++) {
		if (!buffers[i]) { continue; }

		const auto offset = offsets ? offsets[i] : 0;
		views[i].BufferLocation = ToInternal(buffers[i])->gpuAddress + offset;
		views[i].SizeInBytes = static_cast<UINT>(buffers[i]->desc.size - offset);
		views[i].StrideInBytes = strides[i];
	}

	GetCommandList(cmd).GetGraphicsCommandList()->IASetVertexBuffers(slot, count, views);
}


void ou::GraphicsDevice_DX12::BindIndexBuffer(const GpuBuffer *buffer, IndexBufferFormat format, uint64_t offset, CommandList cmd) {
	D3D12_INDEX_BUFFER_VIEW view{};

	if (buffer) {
		view.BufferLocation = ToInternal(buffer)->gpuAddress + offset;
		view.SizeInBytes = static_cast<UINT>(buffer->desc.size - offset);
		view.Format = (format == IndexBufferFormat::UINT16)
						  ? DXGI_FORMAT_R16_UINT
						  : DXGI_FORMAT_R32_UINT;
	}

	GetCommandList(cmd).GetGraphicsCommandList()->IASetIndexBuffer(&view);
}


void ou::GraphicsDevice_DX12::BindConstantBuffer(const GpuBuffer *buffer, uint32_t slot, CommandList cmd, uint64_t offset) {
	ASSERT(slot < DESCRIPTORBINDER_CBV_COUNT);

	GetCommandList(cmd).GetGraphicsCommandList()->SetGraphicsRootConstantBufferView(
		ROOT_CBV_BEGIN + slot, ToInternal(buffer)->gpuAddress + offset);
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
	GetCommandList(cmd).GetGraphicsCommandList()->DrawInstanced(
		vertexCount, 1, startVertexLocation, 0);
}


void ou::GraphicsDevice_DX12::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd) {
	GetCommandList(cmd).GetGraphicsCommandList()->DrawIndexedInstanced(
		indexCount, 1, startIndexLocation, baseVertexLocation, 0);
}


void ou::GraphicsDevice_DX12::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) {
	GetCommandList(cmd).GetGraphicsCommandList()->DrawInstanced(
		vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
}


void ou::GraphicsDevice_DX12::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd) {
	GetCommandList(cmd).GetGraphicsCommandList()->DrawIndexedInstanced(
		indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}


void ou::GraphicsDevice_DX12::Barrier(const GpuBarrier *barriers, uint32_t numBarriers, CommandList cmd) {
	std::vector<D3D12_RESOURCE_BARRIER> d3dBarriers;
	d3dBarriers.reserve(numBarriers);

	for (uint32_t i = 0; i < numBarriers; i++) {
		const auto &barrier = barriers[i];
		D3D12_RESOURCE_BARRIER d3d{};

		switch (barrier.type) {
			case GpuBarrier::Type::MEMORY:
				{
					d3d.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
					d3d.UAV.pResource = barrier.memory.resource
											? ToInternal(barrier.memory.resource)->resource.Get()
											: nullptr;
					break;
				}
			case GpuBarrier::Type::IMAGE:
				{
					const auto *texture = barrier.image.texture;
					const auto before = ConvertResourceState(barrier.image.layoutBefore);
					const auto after = ConvertResourceState(barrier.image.layoutAfter);
					if (before == after) { continue; }

					d3d.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					d3d.Transition.pResource = ToInternal(texture)->resource.Get();
					d3d.Transition.StateBefore = before;
					d3d.Transition.StateAfter = after;
					if (barrier.image.mip < 0 || barrier.image.slice < 0) {
						d3d.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					} else {
						d3d.Transition.Subresource = barrier.image.mip + barrier.image.slice * texture->desc.mipLevels;
					}

					break;
				}
			case GpuBarrier::Type::BUFFER:
				{
					const auto before = ConvertResourceState(barrier.buffer.layoutBefore);
					const auto after = ConvertResourceState(barrier.buffer.layoutAfter);
					if (before == after) { continue; }

					d3d.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					d3d.Transition.pResource = ToInternal(barrier.buffer.buffer)->resource.Get();
					d3d.Transition.StateBefore = before;
					d3d.Transition.StateAfter = after;
					d3d.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

					break;
				}
		}

		d3dBarriers.push_back(d3d);
	}

	if (!d3dBarriers.empty()) {
		GetCommandList(cmd).GetGraphicsCommandList()->ResourceBarrier(
			static_cast<UINT>(d3dBarriers.size()), d3dBarriers.data());
	}
}


GraphicsDevice_DX12::CommandList_DX12 &ou::GraphicsDevice_DX12::GetCommandList(CommandList cmd) const {
	ASSERT(cmd.IsValid());
	return *static_cast<CommandList_DX12 *>(cmd.internalState);
}


GraphicsDevice::GpuLinearAllocator &ou::GraphicsDevice_DX12::GetFrameAllocator(CommandList cmd) {
	auto &cmdDX12 = GetCommandList(cmd);
	return cmdDX12.frameAllocators[cmdDX12.bufferIndex];
}


D3D12_CPU_DESCRIPTOR_HANDLE ou::GraphicsDevice_DX12::GetBindlessCpuHandle(int index) const {
	auto handle = descriptorHeapRes_->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(index) * resDescriptorSize_;
	return handle;
}



void ou::GraphicsDevice_DX12::DescriptorAllocator::Initialize(ID3D12Device *d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorHeapBlock) {
	device = d3dDevice;
	desc.Type = type;
	desc.NumDescriptors = numDescriptorHeapBlock;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // シェーダ不可視
	descriptorSize = device->GetDescriptorHandleIncrementSize(type);
}


D3D12_CPU_DESCRIPTOR_HANDLE ou::GraphicsDevice_DX12::DescriptorAllocator::Allocate() {
	std::scoped_lock lock(locker);

	if (freeList.empty()) {
		BlockAllocate();
	}

	const auto handle = freeList.back();
	freeList.pop_back();
	return handle;
}


void ou::GraphicsDevice_DX12::DescriptorAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	std::scoped_lock lock(locker);
	freeList.push_back(handle);
}


void ou::GraphicsDevice_DX12::DescriptorAllocator::BlockAllocate() {
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &heap = heaps.emplace_back();
	auto hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
	CheckHResult(hr, "ディスクリプタヒープの作成");

	const auto cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < desc.NumDescriptors; i++) {
		auto handle = cpuStart;
		handle.ptr += static_cast<SIZE_T>(i) * descriptorSize;
		freeList.push_back(handle);
	}
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

	// 番号は破棄せず、フリーリストに戻す
	while (!destroyBindlessRes.empty() && destroyBindlessRes.front().second + bufferCount < frameCount) {
		freeBindlessRes.push_back(destroyBindlessRes.front().first);
		destroyBindlessRes.pop_front();
	}
}


int ou::GraphicsDevice_DX12::AllocationHandler::AllocateResourceIndex() {
	std::scoped_lock lock(destroyLocker);

	// プールに空きがなければ -1 を返す
	if (freeBindlessRes.empty()) { return -1; }

	const auto index = freeBindlessRes.back();
	freeBindlessRes.pop_back();
	return index;
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
	frameAllocators[newBufferIndex].Reset();
	activePso = nullptr;
}


ID3D12CommandAllocator *ou::GraphicsDevice_DX12::CommandList_DX12::GetCommandAllocator() const {
	return allocators[bufferIndex][queue].Get();
}


ID3D12GraphicsCommandList *ou::GraphicsDevice_DX12::CommandList_DX12::GetGraphicsCommandList() const {
	return lists[queue].Get();
}


void ou::GraphicsDevice_DX12::CopyAllocator::Initialize(GraphicsDevice_DX12 *owner) {
	device = owner;
}


GraphicsDevice_DX12::CopyAllocator::CopyCMD ou::GraphicsDevice_DX12::CopyAllocator::Allocate(uint64_t stagingSize) {
	HRESULT hr{};
	CopyCMD cmd;

	// プールから大きさが足りるものを探す
	{
		std::scoped_lock lock(locker);
		for (size_t i = 0; i < freeList.size(); i++) {
			if (freeList[i].uploadBuffer.desc.size >= stagingSize) {
				cmd = std::move(freeList[i]);
				freeList.erase(freeList.begin() + i);
				break;
			}
		}
	}

	// なければ新規作成
	if (!cmd.IsValid()) {
		hr = device->device_->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&cmd.allocator));
		CheckHResult(hr, "コピー用アロケータの作成");

		hr = device->device_->CreateCommandList(
			0, D3D12_COMMAND_LIST_TYPE_COPY, cmd.allocator.Get(), nullptr, IID_PPV_ARGS(&cmd.list));
		CheckHResult(hr, "コピー用コマンドリストの作成");
		cmd.list->Close();

		hr = device->device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&cmd.fence));
		CheckHResult(hr, "コピー用フェンスの作成");

		GpuBufferDesc uploadDesc{};
		uploadDesc.size = std::bit_ceil(stagingSize);
		uploadDesc.heapType = HeapType::UPLOAD;
		if (!device->CreateBuffer(&uploadDesc, nullptr, &cmd.uploadBuffer)) {
			FatalError("ステージングバッファの作成に失敗しました。");
		}
		device->SetName(&cmd.uploadBuffer, "CopyAllocator::uploadBuffer");
	}

	// 記録を始める
	hr = cmd.allocator->Reset();
	CheckHResult(hr, "コピー用アロケータのリセット");
	hr = cmd.list->Reset(cmd.allocator.Get(), nullptr);
	CheckHResult(hr, "コピー用コマンドリストのリセット");

	return cmd;
}


void ou::GraphicsDevice_DX12::CopyAllocator::Submit(CopyCMD cmd) {
	HRESULT hr = cmd.list->Close();
	CheckHResult(hr, "コピー用コマンドリストのクローズ");

	auto *queue = device->queues_[QUEUE_COPY].queue.Get();
	ID3D12CommandList *lists[] = {cmd.list.Get()};
	queue->ExecuteCommandLists(1, lists);

	// コピーが終わるまで CPU で待つ
	hr = queue->Signal(cmd.fence.Get(), 1);
	CheckHResult(hr, "コピー用フェンスのシグナル");
	if (cmd.fence->GetCompletedValue() < 1) {
		hr = cmd.fence->SetEventOnCompletion(1, nullptr);
		CheckHResult(hr, "コピー用フェンスの待機");
	}
	cmd.fence->Signal(0);

	// 再利用できるように返す
	std::scoped_lock lock(locker);
	freeList.push_back(std::move(cmd));
}
