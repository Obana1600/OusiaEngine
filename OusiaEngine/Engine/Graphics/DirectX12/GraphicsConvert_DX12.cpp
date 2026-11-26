#include "GraphicsConvert_DX12.h"


DXGI_FORMAT ou::Graphics::ConvertFormat(Format format) {
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
		case Format::R16G16B16A16_FLOAT:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case Format::R11G11B10_FLOAT:
			return DXGI_FORMAT_R11G11B10_FLOAT;

		case Format::R32_UINT:
			return DXGI_FORMAT_R32_UINT;
		case Format::R16_UINT:
			return DXGI_FORMAT_R16_UINT;

		case Format::R8_UNORM:
			return DXGI_FORMAT_R8_UNORM;

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


DXGI_FORMAT ou::Graphics::ConvertFormatNonSrgb(DXGI_FORMAT format) {
	switch (format) {
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM;

		default:
			return format;
	}
}


DXGI_FORMAT ou::Graphics::ConvertFormatToTypeless(DXGI_FORMAT format) {
	switch (format) {
		case DXGI_FORMAT_D32_FLOAT:
			return DXGI_FORMAT_R32_TYPELESS;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			return DXGI_FORMAT_R24G8_TYPELESS;

		default:
			return format;
	}
}


DXGI_FORMAT ou::Graphics::ConvertFormatDepthToSrv(DXGI_FORMAT format) {
	switch (format) {
		case DXGI_FORMAT_D32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

		default:
			return format;
	}
}


D3D12_FILL_MODE ou::Graphics::ConvertFillMode(FillMode value) {
	switch (value) {
		case FillMode::WIREFRAME:
			return D3D12_FILL_MODE_WIREFRAME;
		case FillMode::SOLID:
			return D3D12_FILL_MODE_SOLID;
	}
	return D3D12_FILL_MODE_SOLID;
}


D3D12_CULL_MODE ou::Graphics::ConvertCullMode(CullMode value) {
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


D3D12_COMPARISON_FUNC ou::Graphics::ConvertComparisonFunc(ComparisonFunc value) {
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


D3D12_BLEND ou::Graphics::ConvertBlend(Blend value) {
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


D3D12_BLEND_OP ou::Graphics::ConvertBlendOp(BlendOp value) {
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


D3D12_INPUT_CLASSIFICATION ou::Graphics::ConvertInputClassification(InputClassification value) {
	switch (value) {
		case InputClassification::PER_VERTEX_DATA:
			return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		case InputClassification::PER_INSTANCE_DATA:
			return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
	}
	return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
}


D3D12_PRIMITIVE_TOPOLOGY_TYPE ou::Graphics::ConvertTopologyType(PrimitiveTopology value) {
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


D3D_PRIMITIVE_TOPOLOGY ou::Graphics::ConvertTopology(PrimitiveTopology value, uint32_t patchControlPoints) {
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


D3D12_RESOURCE_STATES ou::Graphics::ConvertResourceState(ResourceState value) {
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
