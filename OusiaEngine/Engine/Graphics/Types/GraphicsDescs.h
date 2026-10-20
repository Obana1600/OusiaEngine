#pragma once

#include "Graphics/Types/GraphicsEnums.h"

#include <string>
#include <vector>
#include <cfloat>
#include <cstdint>


// グラフィクスの設定
namespace ou::Graphics {

// 前方宣言
struct Shader;


// クリアする値
union ClearValue {
	struct ClearDepthStencil {
		float depth;
		uint32_t stencil;
	};

	ClearDepthStencil depthStencil;
	float color[4];
};


// インプットエレメント
struct InputLayout {
	static constexpr uint32_t APPEND_ALIGNED_ELEMENT = ~0u;

	struct Element {
		std::string semanticName;
		uint32_t semanticIndex = 0;
		Format format = Format::UNKNOWN;
		uint32_t inputSlot = 0;
		uint32_t alignedByteOffset = APPEND_ALIGNED_ELEMENT;
		InputClassification inputSlotClass = InputClassification::PER_VERTEX_DATA;
	};
	std::vector<Element> elements;
};


// ラスタライザ
struct RasterizerState {
	FillMode fillMode = FillMode::SOLID;
	CullMode cullMode = CullMode::NONE;
	bool frontCounterClockwise = false;
	int32_t depthBias = 0;
	float depthBiasClamp = 0.0f;
	float slopeScaledDepthBias = 0.0f;
	bool depthClipEnable = false;
	bool multisampleEnable = false;
	bool antialiasedLineEnable = false;
	bool conservativeRasterizationEnable = false;
};


// 深度ステンシル
struct DepthStencilState {
	bool depthEnable = false;
	DepthWriteMask depthWriteMask = DepthWriteMask::ZERO;
	ComparisonFunc comparisonFunc = ComparisonFunc::NEVER;
};


// ブレンド
struct BlendState {
	struct RenderTargetBlendState {
		bool blendEnable = false;
		Blend srcBlend = Blend::SRC_ALPHA;
		Blend destBlend = Blend::INV_SRC_ALPHA;
		BlendOp blendOp = BlendOp::ADD;
		Blend srcBlendAlpha = Blend::ONE;
		Blend destBlendAlpha = Blend::ONE;
		BlendOp blendOpAlpha = BlendOp::ADD;
		ColorWrite renderTargetWriteMask = ColorWrite::ENABLE_ALL;
	};

	RenderTargetBlendState renderTarget[8]; // 1 回の描画で設定できる RenderTarget の数は 8
	bool alphaToCoverageEnable = false;
	bool independentBlendEnable = false;
};


// バッファ
struct GpuBufferDesc {
	uint64_t size = 0;
	HeapType heapType = HeapType::DEFAULT;
	BindFlag bindFlags = BindFlag::NONE;
	ResourceMiscFlag miscFlags = ResourceMiscFlag::NONE;
	uint32_t stride = 0;			 // StructuredBuffer の 1 要素のサイズ
	Format format = Format::UNKNOWN; // 型付きバッファのフォーマット
};


// テクスチャ
struct TextureDesc {
	enum class Type : uint8_t {
		TEXTURE_1D,
		TEXTURE_2D,
		TEXTURE_3D,
	};

	Type type = Type::TEXTURE_2D;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 1;
	uint32_t arraySize = 1;
	uint32_t mipLevels = 1;
	Format format = Format::UNKNOWN;
	uint32_t sampleCount = 1;
	HeapType heapType = HeapType::DEFAULT;
	BindFlag bindFlags = BindFlag::NONE;
	ResourceMiscFlag miscFlags = ResourceMiscFlag::NONE;
	ClearValue clear = {};
	ResourceState layout = ResourceState::SHADER_RESOURCE;
};


// サンプラー
struct SamplerDesc {
	Filter filter = Filter::MIN_MAG_MIP_POINT;
	TextureAddressMode addressU = TextureAddressMode::CLAMP;
	TextureAddressMode addressV = TextureAddressMode::CLAMP;
	TextureAddressMode addressW = TextureAddressMode::CLAMP;
	float mipLodBias = 0.0f;
	uint32_t maxAnisotropy = 0;
	ComparisonFunc comparisonFunc = ComparisonFunc::NEVER;
	SamplerBorderColor borderColor = SamplerBorderColor::TRANSPARENT_BLACK;
	float minLod = 0.0f;
	float maxLod = FLT_MAX;
};


// スワップチェーン
struct SwapChainDesc {
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t bufferCount = 2;
	Format format = Format::R8G8B8A8_UNORM;
	bool fullscreen = false;
	bool vsync = true;
	float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};


// パイプライン
struct PipelineStateDesc {
	const Shader *vs = nullptr;
	const Shader *ps = nullptr;
	const Shader *hs = nullptr;
	const Shader *ds = nullptr;
	const Shader *gs = nullptr;
	const Shader *ms = nullptr;
	const Shader *as = nullptr;

	const BlendState *bs = nullptr;
	const RasterizerState *rs = nullptr;
	const DepthStencilState *dss = nullptr;

	const InputLayout *il = nullptr;
	PrimitiveTopology pt = PrimitiveTopology::TRIANGLELIST;
	uint32_t patchControlPoints = 3; // テッセレーションで使う
	uint32_t sampleMask = 0xFFFFFFFF;
};

} // namespace ou::Graphics
