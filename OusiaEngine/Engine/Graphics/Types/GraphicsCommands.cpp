#include "GraphicsCommands.h"

using namespace ou;


Graphics::GpuBarrier ou::Graphics::GpuBarrier::Memory(const GpuResource *resource) {
	GpuBarrier barrier;
	barrier.type = Type::MEMORY;
	barrier.memory.resource = resource;
	return barrier;
}


Graphics::GpuBarrier ou::Graphics::GpuBarrier::Image(const Texture *texture,
													 ResourceState before, ResourceState after,
													 int mip, int slice) {
	GpuBarrier barrier;
	barrier.type = Type::IMAGE;
	barrier.image.texture = texture;
	barrier.image.layoutBefore = before;
	barrier.image.layoutAfter = after;
	barrier.image.mip = mip;
	barrier.image.slice = slice;
	return barrier;
}


Graphics::GpuBarrier ou::Graphics::GpuBarrier::Buffer(const GpuBuffer *buffer,
													  ResourceState before, ResourceState after) {
	GpuBarrier barrier;
	barrier.type = Type::BUFFER;
	barrier.buffer.buffer = buffer;
	barrier.buffer.layoutBefore = before;
	barrier.buffer.layoutAfter = after;
	return barrier;
}


Graphics::RenderPassImage ou::Graphics::RenderPassImage::RenderTarget(const Texture *texture,
																	  LoadOp loadOp, StoreOp storeOp,
																	  ResourceState before, ResourceState after,
																	  int subresource) {
	RenderPassImage image;
	image.type = Type::RENDERTARGET;
	image.texture = texture;
	image.loadOp = loadOp;
	image.storeOp = storeOp;
	image.layoutBefore = before;
	image.layout = ResourceState::RENDERTARGET;
	image.layoutAfter = after;
	image.subresource = subresource;
	return image;
}


Graphics::RenderPassImage ou::Graphics::RenderPassImage::DepthStencil(const Texture *texture,
																	  LoadOp loadOp, StoreOp storeOp,
																	  ResourceState before, ResourceState current, ResourceState after,
																	  int subresource) {
	RenderPassImage image;
	image.type = Type::DEPTH_STENCIL;
	image.texture = texture;
	image.loadOp = loadOp;
	image.storeOp = storeOp;
	image.layoutBefore = before;
	image.layout = current;
	image.layoutAfter = after;
	image.subresource = subresource;
	return image;
}


Graphics::RenderPassInfo ou::Graphics::RenderPassInfo::From(const SwapChainDesc &desc) {
	RenderPassInfo info;
	info.rtFormats[0] = desc.format;
	info.rtCount = 1;
	return info;
}


Graphics::RenderPassInfo ou::Graphics::RenderPassInfo::From(const RenderPassImage *images, uint32_t imageCount) {
	RenderPassInfo info;
	for (uint32_t i = 0; i < imageCount; i++) {
		const auto &image = images[i];
		switch (image.type) {
			case RenderPassImage::Type::RENDERTARGET:
				info.rtFormats[info.rtCount++] = image.texture->desc.format;
				break;
			case RenderPassImage::Type::DEPTH_STENCIL:
				info.dsFormat = image.texture->desc.format;
				break;
		}
		info.sampleCount = image.texture->desc.sampleCount;
	}
	return info;
}
