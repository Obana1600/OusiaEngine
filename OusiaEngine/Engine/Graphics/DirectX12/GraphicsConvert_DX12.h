#pragma once

#include "Graphics/Types/GraphicsEnums.h"

#include <cstdint>

#include <d3d12.h>
#include <dxgiformat.h>


namespace ou::Graphics {

// Format -> DXGI_FORMAT
DXGI_FORMAT ConvertFormat(Format format);

//
DXGI_FORMAT ConvertFormatNonSrgb(DXGI_FORMAT format);

DXGI_FORMAT ConvertFormatToTypeless(DXGI_FORMAT format);

DXGI_FORMAT ConvertFormatDepthToSrv(DXGI_FORMAT format);

D3D12_FILL_MODE ConvertFillMode(FillMode value);

D3D12_CULL_MODE ConvertCullMode(CullMode value);

D3D12_COMPARISON_FUNC ConvertComparisonFunc(ComparisonFunc value);

D3D12_BLEND ConvertBlend(Blend value);

D3D12_BLEND_OP ConvertBlendOp(BlendOp value);

D3D12_INPUT_CLASSIFICATION ConvertInputClassification(InputClassification value);

D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertTopologyType(PrimitiveTopology value);

D3D_PRIMITIVE_TOPOLOGY ConvertTopology(PrimitiveTopology value, uint32_t patchControlPoints);

D3D12_RESOURCE_STATES ConvertResourceState(ResourceState value);

} // namespace ou::Graphics
