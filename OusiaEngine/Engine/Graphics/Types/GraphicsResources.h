#pragma once

#include "Graphics/Types/GraphicsDescs.h"
#include "Graphics/Types/GraphicsEnums.h"

#include <cstdint>
#include <memory>


namespace ou::Graphics {

// デバイスが作るすべてのオブジェクトの基底
struct GraphicsDeviceChild {
	std::shared_ptr<void> internalState;

	bool IsValid() const { return internalState != nullptr; }
};


// サンプラー
struct Sampler: public GraphicsDeviceChild {
	SamplerDesc desc;

	const SamplerDesc &GetDesc() const { return desc; }
};


// シェーダー
struct Shader: public GraphicsDeviceChild {
	ShaderStage stage = ShaderStage::Count;
};


// リソース
struct GpuResource: public GraphicsDeviceChild {
	enum class Type : uint8_t {
		BUFFER,
		TEXTURE,
		UNKNOWN,
	};

	Type type = Type::UNKNOWN;
	void *mappedData = nullptr;
	size_t mappedSize = 0;

	bool IsBuffer() const { return type == Type::BUFFER; }
	bool IsTexture() const { return type == Type::TEXTURE; }
};


// バッファー
struct GpuBuffer: public GpuResource {
	GpuBufferDesc desc;

	const GpuBufferDesc &GetDesc() const { return desc; }
};


// テクスチャ
struct Texture: public GpuResource {
	TextureDesc desc;

	const TextureDesc &GetDesc() const { return desc; }
};


// パイプライン
struct PipelineState: public GraphicsDeviceChild {
	PipelineStateDesc desc;

	const PipelineStateDesc &GetDesc() const { return desc; }
};


// スワップチェーン
struct SwapChain: public GraphicsDeviceChild {
	SwapChainDesc desc;

	const SwapChainDesc &GetDesc() const { return desc; }
};


// ビューポート
struct Viewport {
	float topLeftX = 0.0f;
	float topLeftY = 0.0f;
	float width = 0.0f;
	float height = 0.0f;
	float minDepth = 0.0f;
	float maxDepth = 1.0f;
};


// シザー矩形
struct Scissor {
	int32_t left = 0;
	int32_t top = 0;
	int32_t right = 0;
	int32_t bottom = 0;
};


// テクスチャの初期データ
struct SubresourceData {
	const void *dataPtr = nullptr;
	uint32_t rowPitch = 0;
	uint32_t slicePitch = 0;
};

} // namespace ou::Graphics
