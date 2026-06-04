#pragma once

#include "math/Vector4.h"
#include "math/Vector3.h"
#include "math/Vector2.h"
#include <d3d12.h>


struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;

	static const D3D12_INPUT_LAYOUT_DESC InputLayout;

private:

	static const int kInputElementCount = 3;
	static const D3D12_INPUT_ELEMENT_DESC kInputElements[kInputElementCount];

};


inline const D3D12_INPUT_ELEMENT_DESC VertexData::kInputElements[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT },
};


inline const D3D12_INPUT_LAYOUT_DESC VertexData::InputLayout = {
	.pInputElementDescs = kInputElements,
	.NumElements = kInputElementCount
};