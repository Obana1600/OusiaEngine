#include "GraphicsDevice.h"

#include <algorithm>
#include <cstring>

using namespace ou;
using namespace ou::Graphics;


GraphicsDevice::GpuAllocation ou::Graphics::GraphicsDevice::AllocateGpu(uint64_t dataSize, CommandList cmd) {
	GpuAllocation allocation;
	if (dataSize == 0) { return allocation; }

	auto &allocator = GetFrameAllocator(cmd);
	const auto alignedSize = AlignTo(dataSize, kAllocationAlignment);
	const auto freeSpace = allocator.buffer.IsValid() ? allocator.buffer.desc.size - allocator.offset : 0;

	// 足りなければ大きいバッファに作り直す
	if (alignedSize > freeSpace) {
		GpuBufferDesc desc;
		desc.size = AlignTo((allocator.buffer.desc.size + alignedSize) * 2, kAllocationAlignment);
		desc.heapType = HeapType::UPLOAD;
		desc.bindFlags = BindFlag::CONSTANT_BUFFER | BindFlag::VERTEX_BUFFER | BindFlag::INDEX_BUFFER;
		if (!CreateBuffer(&desc, nullptr, &allocator.buffer)) { return allocation; }
		SetName(&allocator.buffer, "FrameAllocator");
		allocator.offset = 0;
	}

	// 今の位置を貸して offset を進める
	allocation.buffer = allocator.buffer;
	allocation.offset = allocator.offset;
	allocation.data = static_cast<uint8_t *>(allocator.buffer.mappedData) + allocator.offset;
	allocator.offset += alignedSize;

	return allocation;
}


void ou::Graphics::GraphicsDevice::UpdateBuffer(const GpuBuffer *buffer, const void *data, CommandList cmd, uint64_t size, uint64_t offset) {
	if (!buffer || !buffer->IsValid() || !data) { return; }
	if (offset >= buffer->desc.size) { return; }

	size = std::min(size, buffer->desc.size - offset);
	auto allocation = AllocateGpu(size, cmd);
	if (!allocation.IsValid()) { return; }

	std::memcpy(allocation.data, data, size);
	CopyBuffer(buffer, offset, &allocation.buffer, allocation.offset, size, cmd);
}
