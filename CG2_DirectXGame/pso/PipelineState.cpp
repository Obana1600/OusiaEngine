#include "PipelineState.h"
#include "DXCompiler.h"
#include <string>
#include <assert.h>

using namespace Microsoft::WRL;


PipelineState::PipelineState() {
	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	// すべての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 裏面（時計回り）を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	// 書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	// 比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// PSOを生成する
	m_desc.BlendState = blendDesc; // BlendState
	m_desc.RasterizerState = rasterizerDesc; // RasterizerState
	// 書き込むRTVの情報
	m_desc.NumRenderTargets = 1;
	m_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// 利用するトロポジ（形状）のタイプ。三角形
	m_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どのように画面に色を打ち込むかの設定（気にしなくていい）
	m_desc.SampleDesc.Count = 1;
	m_desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	// DepthStencilの設定
	m_desc.DepthStencilState = depthStencilDesc;
	m_desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
}


void PipelineState::SetVS(const std::wstring &filePath, const wchar_t *profile) {
	m_VSBlob = DXCompiler::GetInstance()->CompileShader(filePath, profile);
	assert(m_VSBlob);

	m_desc.VS = { m_VSBlob->GetBufferPointer(), m_VSBlob->GetBufferSize() };
}


void PipelineState::SetPS(const std::wstring &filePath, const wchar_t *profile) {
	m_PSBlob = DXCompiler::GetInstance()->CompileShader(filePath, profile);
	assert(m_PSBlob);

	m_desc.PS = { m_PSBlob->GetBufferPointer(), m_PSBlob->GetBufferSize() };
}


void PipelineState::Create(const ComPtr<ID3D12Device> &device) {
	// 実際に生成
	HRESULT hr = device->CreateGraphicsPipelineState(&m_desc, IID_PPV_ARGS(&m_pipelineState));
	assert(SUCCEEDED(hr));

	m_isValid = true;
}
