#include "GraphicsBuffer.h"
#include "Graphics/GraphicsDevice.h"
#include "Render/ImmediateCommands.h"
#include "Diagnostics/Log.h"

using namespace ou;


ou::GraphicsBuffer::GraphicsBuffer(Target target, int count, int stride)
	: target_(target),
	  count_(count),
	  stride_(stride) {
	if (count <= 0 || stride <= 0) {
		Log::Error("GraphicsBuffer の count と stride は 1 以上にしてください (count={}, stride={})", count, stride);
		return;
	}
	if (HasFlag(target, Target::Structured) && HasFlag(target, Target::Raw)) {
		Log::Error("GraphicsBuffer の Target に Structured と Raw を一緒に指定することはできません");
		return;
	}

	Graphics::GpuBufferDesc desc{};
	desc.size = static_cast<uint64_t>(count) * static_cast<uint64_t>(stride);
	desc.stride = static_cast<uint32_t>(stride);
	if (HasFlag(target, Target::Vertex)) { desc.bindFlags |= Graphics::BindFlag::VERTEX_BUFFER; }
	if (HasFlag(target, Target::Index)) { desc.bindFlags |= Graphics::BindFlag::INDEX_BUFFER; }

	if (HasFlag(target, Target::Structured)) {
		desc.bindFlags |= Graphics::BindFlag::SHADER_RESOURCE | Graphics::BindFlag::UNORDERED_ACCESS;
		desc.miscFlags |= Graphics::ResourceMiscFlag::BUFFER_STRUCTURED;
	}
	if (HasFlag(target, Target::Raw)) {
		if (stride % 4 != 0) {
			Log::Error("Raw の GraphicsBuffer の stride は 4 の倍数にしてください (stride={})", stride);
			return;
		}
		desc.bindFlags |= Graphics::BindFlag::SHADER_RESOURCE | Graphics::BindFlag::UNORDERED_ACCESS;
		desc.miscFlags |= Graphics::ResourceMiscFlag::BUFFER_RAW;
	}

	if (!Graphics::GetDevice()->CreateBuffer(&desc, nullptr, &buffer_)) {
		Log::Error("GraphicsBuffer の作成に失敗しました");
		buffer_ = {};
	}
}


void ou::GraphicsBuffer::Release() {
	buffer_ = {};
}


bool ou::GraphicsBuffer::IsValid() const {
	return buffer_.IsValid();
}


int ou::GraphicsBuffer::GetCount() const {
	return count_;
}


int ou::GraphicsBuffer::GetStride() const {
	return stride_;
}


GraphicsBuffer::Target ou::GraphicsBuffer::GetTarget() const {
	return target_;
}


void ou::GraphicsBuffer::SetName(std::string_view name) {
	name_ = name;
	if (IsValid()) { Graphics::GetDevice()->SetName(&buffer_, name_); }
}


const std::string &ou::GraphicsBuffer::GetName() const {
	return name_;
}


void ou::GraphicsBuffer::SetDataBytes(const void *data, size_t byteOffset, size_t byteSize) {
	if (!IsValid()) {
		Log::Error("作られていない GraphicsBuffer \"{}\" に SetData しました", name_);
		return;
	}
	if (byteSize == 0) { return; }
	if (byteOffset + byteSize > buffer_.desc.size) {
		Log::Error("GraphicsBuffer \"{}\" の大きさ ({} バイト) を超えて SetData しました (位置 {} バイト, {} バイト)",
				   name_, buffer_.desc.size, byteOffset, byteSize);
		return;
	}

	auto *device = Graphics::GetDevice();
	const auto cmd = ImmediateCommands::Acquire();

	using State = Graphics::ResourceState;
	const auto toCopy = Graphics::GpuBarrier::Buffer(&buffer_, State::UNDEFINED, State::COPY_DST);
	device->Barrier(&toCopy, 1, cmd);

	device->UpdateBuffer(&buffer_, data, cmd, byteSize, byteOffset);

	const auto toCommon = Graphics::GpuBarrier::Buffer(&buffer_, State::COPY_DST, State::UNDEFINED);
	device->Barrier(&toCommon, 1, cmd);
}