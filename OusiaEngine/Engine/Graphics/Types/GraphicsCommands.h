#pragma once

#include "Graphics/Types/GraphicsDescs.h"
#include "Graphics/Types/GraphicsEnums.h"
#include "Graphics/Types/GraphicsResources.h"

#include <cstdint>


namespace ou::Graphics {

// コマンドリストのハンドル
struct CommandList {
	void *internalState = nullptr;

	constexpr bool IsValid() const { return internalState != nullptr; }
};


// リソースバリア
struct GpuBarrier {
	enum class Type : uint8_t {
		MEMORY,
		IMAGE,
		BUFFER,
	};

	struct Memory {
		const GpuResource *resource;
	};
	struct Image {
		const Texture *texture;
		ResourceState layoutBefore;
		ResourceState layoutAfter;
		int mip;
		int slice;
	};
	struct Buffer {
		const GpuBuffer *buffer;
		ResourceState layoutBefore;
		ResourceState layoutAfter;
	};


	Type type = Type::MEMORY;
	union {
		Memory memory;
		Image image;
		Buffer buffer;
	};


	static GpuBarrier Memory(const GpuResource *resource = nullptr);

	static GpuBarrier Image(const Texture *texture,
							ResourceState before, ResourceState after,
							int mip = -1, int slice = -1);

	static GpuBarrier Buffer(const GpuBuffer *buffer,
							 ResourceState before, ResourceState after);
};


// レンダーパス
struct RenderPassImage {
	enum class Type : uint8_t {
		RENDERTARGET,
		DEPTH_STENCIL,
	};
	enum class LoadOp : uint8_t {
		LOAD,
		CLEAR,
		DONTCARE,
	};
	enum class StoreOp : uint8_t {
		STORE,
		DONTCARE,
	};


	Type type = Type::RENDERTARGET;
	LoadOp loadOp = LoadOp::LOAD;
	StoreOp storeOp = StoreOp::STORE;

	const Texture *texture = nullptr;
	int subresource = -1;

	ResourceState layoutBefore = ResourceState::UNDEFINED;
	ResourceState layout = ResourceState::UNDEFINED;
	ResourceState layoutAfter = ResourceState::UNDEFINED;


	static RenderPassImage RenderTarget(
		const Texture *texture,
		LoadOp loadOp = LoadOp::LOAD,
		StoreOp storeOp = StoreOp::STORE,
		ResourceState before = ResourceState::SHADER_RESOURCE,
		ResourceState after = ResourceState::SHADER_RESOURCE,
		int subresource = -1);

	static RenderPassImage DepthStencil(
		const Texture *texture,
		LoadOp loadOp = LoadOp::LOAD,
		StoreOp storeOp = StoreOp::STORE,
		ResourceState before = ResourceState::DEPTHSTENCIL,
		ResourceState current = ResourceState::DEPTHSTENCIL,
		ResourceState after = ResourceState::DEPTHSTENCIL,
		int subresource = -1);
};


// PSO が書き込む先のフォーマット
struct RenderPassInfo {
	Format rtFormats[8] = {}; // RenderTarget の数は 8 が上限
	uint32_t rtCount = 0;
	Format dsFormat = Format::UNKNOWN;
	uint32_t sampleCount = 1;

	static RenderPassInfo From(const SwapChainDesc &desc);
	static RenderPassInfo From(const RenderPassImage *images, uint32_t imageCount);

	bool operator==(const RenderPassInfo &) const = default;
};

} // namespace ou::Graphics
