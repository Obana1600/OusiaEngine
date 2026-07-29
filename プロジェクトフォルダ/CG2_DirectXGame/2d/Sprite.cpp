#include "Sprite.h"

#include "base/CreateBufferResource.h"
#include "base/DXCompiler.h"
#include "base/DirectXCommon.h"
#include "base/TextureManager.h"
#include "math/MathUtility.h"
#include <cassert>
#include <utility>

using namespace MathUtility;


void Sprite::StaticInitialize(
	ID3D12Device *device,
	int windowWidth,
	int windowHeight,
	const std::wstring &directoryPath
) {
	sDevice_ = device;
	sDescriptorHeapIncrementSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 正射影行列
	sMatProjection_ = MathUtility::MakeOrthographicMatrix(
		0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.0f, 100.0f
	);


	// ルートシグネチャ ------------------------------------------------------------------------------

	D3D12_ROOT_SIGNATURE_DESC sigDesc{};
	sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// srvレンジ
	D3D12_DESCRIPTOR_RANGE range[1]{};
	range[0].BaseShaderRegister = 0;
	range[0].NumDescriptors = 1;
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER params[2]{};
	// b0
	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	params[0].Descriptor.ShaderRegister = 0;

	// texture: t0
	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	params[1].DescriptorTable.pDescriptorRanges = range;
	params[1].DescriptorTable.NumDescriptorRanges = _countof(range);

	sigDesc.pParameters = params;
	sigDesc.NumParameters = _countof(params);

	// sampler: s0
	D3D12_STATIC_SAMPLER_DESC sampler[1]{};
	sampler[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler[0].MaxLOD = D3D12_FLOAT32_MAX;
	sampler[0].ShaderRegister = 0;
	sampler[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	sigDesc.pStaticSamplers = sampler;
	sigDesc.NumStaticSamplers = _countof(sampler);

	// シリアライズ -> 生成
	Microsoft::WRL::ComPtr<ID3DBlob> sig;
	Microsoft::WRL::ComPtr<ID3DBlob> err;

	HRESULT hr = D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err);
	if (FAILED(hr)) {
		if (err) {
			OutputDebugStringA(reinterpret_cast<char *>(err->GetBufferPointer()));
		}
		assert(false);
	}

	hr = device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&sRootSignature_));
	assert(SUCCEEDED(hr));

	// --------------------------------------------------------------------------------------------


	// pso ----------------------------------------------------------------------------------------

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};

	D3D12_INPUT_ELEMENT_DESC inputElements[2]{};
	// POSITION: float3
	inputElements[0].SemanticName = "POSITION";
	inputElements[0].SemanticIndex = 0;
	inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	// TEXTCOORD: float2
	inputElements[1].SemanticName = "TEXCOORD";
	inputElements[1].SemanticIndex = 0;
	inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	// inputLayout
	D3D12_INPUT_LAYOUT_DESC inputLayout{};
	inputLayout.pInputElementDescs = inputElements;
	inputLayout.NumElements = _countof(inputElements);
	psoDesc.InputLayout = inputLayout;

	// blendState: アルファブレンドのみ
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState = blendDesc;

	// depthStencil
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = FALSE;
	psoDesc.DepthStencilState = depthStencilDesc;

	// rasterizerState
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState = rasterizerDesc;

	// shader
	Microsoft::WRL::ComPtr<IDxcBlob> vsBlob =
		DXCompiler::GetInstance()->CompileShader(L"Sprite.VS.hlsl", L"vs_6_0");
	assert(vsBlob);
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob =
		DXCompiler::GetInstance()->CompileShader(L"Sprite.PS.hlsl", L"ps_6_0");
	assert(psBlob);

	psoDesc.pRootSignature = sRootSignature_.Get();
	psoDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
	psoDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&sPipelineState_));
	assert(SUCCEEDED(hr));

	// --------------------------------------------------------------------------------------------
}


void Sprite::PreDraw(ID3D12GraphicsCommandList *commandList) {
	if (commandList) {
		sCommandList_ = commandList;
	} else {
		sCommandList_ = DirectXCommon::GetInstance()->GetCommandListForRender();
	}

	sCommandList_->SetGraphicsRootSignature(sRootSignature_.Get());
	sCommandList_->SetPipelineState(sPipelineState_.Get());
	sCommandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}


void Sprite::PostDraw() {
	sCommandList_ = nullptr;
}


Sprite *Sprite::Create(
	uint32_t textureHandle,
	Vector2 position,
	Vector4 color,
	Vector2 anchorPoint,
	bool isFlipX,
	bool isFlipY
) {
	Sprite *sprite = new Sprite(textureHandle, position, {0.0f, 0.0f}, color, anchorPoint, isFlipX, isFlipY);
	sprite->Initialize();
	return sprite;
}


bool Sprite::Initialize() {
	assert(sDevice_);

	SetTextureHandle(textureHandle_);

	// テクスチャの実寸初期化
	texBase_ = {0.0f, 0.0f};
	texSize_ = {static_cast<float>(resourceDesc_.Width), static_cast<float>(resourceDesc_.Height)};
	if (size_.x == 0.0f && size_.y == 0.0f) {
		size_ = texSize_;
	}

	// 頂点バッファ
	size_t vbSize = sizeof(VertexPosUv) * kNumVertex;
	vertexResource_ = CreateBufferResource(sDevice_, vbSize);
	HRESULT hr = vertexResource_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData_));
	assert(SUCCEEDED(hr));

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = UINT(vbSize);
	vertexBufferView_.StrideInBytes = sizeof(VertexPosUv);

	// 定数バッファ
	size_t cbSize = (sizeof(ConstBufferData) + 255) & ~size_t(255);
	constResource_ = CreateBufferResource(sDevice_, cbSize);
	hr = constResource_->Map(0, nullptr, reinterpret_cast<void **>(&constData_));
	assert(SUCCEEDED(hr));

	TransferVertex();

	return true;
}


void Sprite::SetTextureHandle(uint32_t textureHandle) {
	textureHandle_ = textureHandle;
	resourceDesc_ = TextureManager::GetInstance()->GetResourceDesc(textureHandle);
}


void Sprite::SetTranslation(const Vector2 &position) {
	translation_ = position;
}


void Sprite::SetRotation(const Vector3 &rotation) {
	rotation_ = rotation;
}


void Sprite::SetSize(const Vector2 &size) {
	size_ = size;
}


void Sprite::SetScale(const Vector2 &scale) {
	scale_ = scale;
}


void Sprite::SetAnchorPoint(const Vector2 &anchorPoint) {
	anchorPoint_ = anchorPoint;
}


void Sprite::SetColor(const Vector4 &color) {
	color_ = color;
}


void Sprite::SetIsFlipX(bool isFlipX) {
	isFlipX_ = isFlipX;
}


void Sprite::SetIsFlipY(bool isFlipY) {
	isFlipY_ = isFlipY;
}


void Sprite::SetTextureRect(const Vector2 &texBase, const Vector2 &texSize) {
	texBase_ = texBase;
	texSize_ = texSize;
}


void Sprite::Draw() {
	assert(sCommandList_);

	TransferVertex();

	sCommandList_->IASetVertexBuffers(0, 1, &vertexBufferView_);
	sCommandList_->SetGraphicsRootConstantBufferView(0, constResource_->GetGPUVirtualAddress());
	TextureManager::GetInstance()->SetGraphicsRootDescriptorTable(sCommandList_, 1, textureHandle_);

	sCommandList_->DrawInstanced(kNumVertex, 1, 0, 0);
}


Sprite::Sprite() = default;


Sprite::Sprite(
	uint32_t textureHandle,
	const Vector2 &position,
	const Vector2 &size,
	const Vector4 &color,
	const Vector2 &anchorPoint,
	bool isFlipX,
	bool isFlipY
)
	: textureHandle_(textureHandle), translation_(position), size_(size), color_(color), anchorPoint_(anchorPoint), isFlipX_(isFlipX), isFlipY_(isFlipY) {
}


void Sprite::TransferVertex() {
	// ワールド行列
	worldMatrix_ = MakeAffineMatrix(
		{scale_.x, scale_.y, 1.0f},
		rotation_,
		{translation_.x, translation_.y, 0.0f}
	);

	// アンカーポイントによる4頂点の計算
	float left = -size_.x * anchorPoint_.x;
	float right = size_.x * (1.0f - anchorPoint_.x);
	float top = -size_.y * anchorPoint_.y;
	float bottom = size_.y * (1.0f - anchorPoint_.y);

	// テクスチャ範囲を正規化
	float texLeft = texBase_.x / static_cast<float>(resourceDesc_.Width);
	float texRight = (texBase_.x + texSize_.x) / static_cast<float>(resourceDesc_.Width);
	float texTop = texBase_.y / static_cast<float>(resourceDesc_.Height);
	float texBottom = (texBase_.y + texSize_.y) / static_cast<float>(resourceDesc_.Height);

	// 反転処理
	if (isFlipX_) {
		std::swap(texLeft, texRight);
	}
	if (isFlipY_) {
		std::swap(texTop, texBottom);
	}

	// z順に並べる
	vertexData_[0] = {{left, top, 0.0f}, {texLeft, texTop}};
	vertexData_[1] = {{right, top, 0.0f}, {texRight, texTop}};
	vertexData_[2] = {{left, bottom, 0.0f}, {texLeft, texBottom}};
	vertexData_[3] = {{right, bottom, 0.0f}, {texRight, texBottom}};

	constData_->color = color_;
	constData_->mat = worldMatrix_ * sMatProjection_;
	constData_->uvTransform = MakeAffineMatrix(
		{uvScale_.x, uvScale_.y, 0.0f},
		{0.0f, 0.0f, uvRotation_},
		{uvTranslation_.x, uvTranslation_.y, 0.0f}
	);
}