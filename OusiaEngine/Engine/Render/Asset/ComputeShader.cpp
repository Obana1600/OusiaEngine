#include "ComputeShader.h"
#include "Diagnostics/Log.h"
#include "Graphics/GraphicsDevice.h"
#include "Render/Asset/RenderTexture.h"
#include "Render/GraphicsBuffer.h"
#include "Render/ImmediateCommands.h"

#include <algorithm>
#include <cstring>

using namespace ou;


int ou::ComputeShader::FindKernel(std::string_view name) const {
	for (size_t i = 0; i < kernels_.size(); i++) {
		if (kernels_[i].name == name) { return static_cast<int>(i); }
	}
	Log::Error("ComputeShader \"{}\" にカーネル \"{}\" はありません", GetName(), name);
	return -1;
}


bool ou::ComputeShader::HasKernel(std::string_view name) const {
	return std::ranges::any_of(kernels_, [&](const auto &kernel) { return kernel.name == name; });
}


void ou::ComputeShader::GetKernelThreadGroupSizes(int kernelIndex, uint32_t &x, uint32_t &y, uint32_t &z) const {
	x = y = z = 0;
	if (!IsValidKernel(kernelIndex)) { return; }

	const auto &size = kernels_[kernelIndex].threadGroupSize;
	x = size[0];
	y = size[1];
	z = size[2];
}


void ou::ComputeShader::SetFloat(std::string_view name, float value) {
	WriteGlobal(name, &value, sizeof(value));
}


void ou::ComputeShader::SetInt(std::string_view name, int value) {
	WriteGlobal(name, &value, sizeof(value));
}


void ou::ComputeShader::SetBool(std::string_view name, bool value) {
	const uint32_t bits = value ? 1u : 0u;
	WriteGlobal(name, &bits, sizeof(bits));
}


void ou::ComputeShader::SetVector(std::string_view name, const Vector4 &value) {
	WriteGlobal(name, &value, sizeof(value));
}


void ou::ComputeShader::SetMatrix(std::string_view name, const Matrix4x4 &value) {
	WriteGlobal(name, &value, sizeof(value));
}


void ou::ComputeShader::SetTexture(int kernelIndex, std::string_view name, Texture *texture) {
	SetResource(kernelIndex, name, texture, nullptr);
}


void ou::ComputeShader::SetBuffer(int kernelIndex, std::string_view name, GraphicsBuffer *buffer) {
	if (buffer && !HasFlag(buffer->GetTarget(), GraphicsBuffer::Target::Structured) &&
		!HasFlag(buffer->GetTarget(), GraphicsBuffer::Target::Raw)) {
		Log::Error("ComputeShader \"{}\" の SetBuffer に渡せるのは Target::Structured か Target::Raw の GraphicsBuffer のみです (\"{}\")", GetName(), name);
		return;
	}
	SetResource(kernelIndex, name, nullptr, buffer);
}


void ou::ComputeShader::Dispatch(int kernelIndex, int threadGroupsX, int threadGroupsY, int threadGroupsZ) {
	if (!IsValidKernel(kernelIndex)) { return; }
	if (threadGroupsX < 0 || threadGroupsY < 0 || threadGroupsZ < 0) {
		Log::Error("ComputeShader \"{}\" の Dispatch のグループ数が負です ({}, {}, {})", GetName(), threadGroupsX, threadGroupsY, threadGroupsZ);
		return;
	}
	if (threadGroupsX == 0 || threadGroupsY == 0 || threadGroupsZ == 0) { return; }

	auto *device = Graphics::GetDevice();
	const auto &kernel = kernels_[kernelIndex];
	auto constants = globals_;

	using State = Graphics::ResourceState;
	std::vector<Graphics::GpuBarrier> toUav;
	std::vector<Graphics::GpuBarrier> fromUav;

	for (const auto &binding : kernel.bindings) {
		int index = -1;
		if (binding.texture) {
			auto *renderTexture = dynamic_cast<RenderTexture *>(binding.texture);
			if (renderTexture && !renderTexture->Create()) { return; }

			const auto &texture = binding.texture->GetGraphicsTexture();
			const bool isUav = renderTexture && renderTexture->GetEnableRandomWrite();
			index = device->GetDescriptorIndex(&texture, isUav ? Graphics::ViewType::UAV : Graphics::ViewType::SRV);
			if (isUav) {
				toUav.push_back(Graphics::GpuBarrier::Image(&texture, State::SHADER_RESOURCE, State::UNORDERED_ACCESS));
				fromUav.push_back(Graphics::GpuBarrier::Image(&texture, State::UNORDERED_ACCESS, State::SHADER_RESOURCE));
			}

		} else if (binding.buffer) {
			if (!binding.buffer->IsValid()) {
				Log::Error("ComputeShader \"{}\" の \"{}\" の GraphicsBuffer は Release されています", GetName(), binding.name);
				return;
			}
			const auto &buffer = binding.buffer->buffer_;
			index = device->GetDescriptorIndex(&buffer, Graphics::ViewType::UAV);
			toUav.push_back(Graphics::GpuBarrier::Buffer(&buffer, State::UNDEFINED, State::UNORDERED_ACCESS));
			fromUav.push_back(Graphics::GpuBarrier::Buffer(&buffer, State::UNORDERED_ACCESS, State::UNDEFINED));
		}

		if (index < 0) {
			Log::Error("ComputeShader \"{}\" の \"{}\" に渡したものの番号がありません", GetName(), binding.name);
			return;
		}
		const auto value = static_cast<uint32_t>(index);
		std::memcpy(constants.data() + binding.offset, &value, sizeof(value));
	}

	const auto cmd = ImmediateCommands::Acquire();
	if (!toUav.empty()) { device->Barrier(toUav.data(), static_cast<uint32_t>(toUav.size()), cmd); }

	device->BindComputeShader(&kernel.shader, cmd);

	if (!constants.empty()) {
		auto allocation = device->AllocateGpu(constants.size(), cmd);
		if (!allocation.IsValid()) { return; }
		std::memcpy(allocation.data, constants.data(), constants.size());
		device->BindConstantBuffer(&allocation.buffer, 0, cmd, allocation.offset);
	}

	device->Dispatch(static_cast<uint32_t>(threadGroupsX), static_cast<uint32_t>(threadGroupsY), static_cast<uint32_t>(threadGroupsZ), cmd);

	if (!fromUav.empty()) { device->Barrier(fromUav.data(), static_cast<uint32_t>(fromUav.size()), cmd); }
}


const ComputeShader::GlobalVariable *ou::ComputeShader::FindGlobalVariable(std::string_view name) const {
	for (const auto &variable : globalVariables_) {
		if (variable.name == name) { return &variable; }
	}
	return nullptr;
}


void ou::ComputeShader::WriteGlobal(std::string_view name, const void *data, uint32_t size) {
	const auto *variable = FindGlobalVariable(name);
	if (!variable) {
		Log::Warning("ComputeShader \"{}\" に変数 \"{}\" がありません", GetName(), name);
		return;
	}

	std::memcpy(globals_.data() + variable->offset, data, std::min(size, variable->size));
}


void ou::ComputeShader::SetResource(int kernelIndex, std::string_view name, Texture *texture, GraphicsBuffer *buffer) {
	if (!IsValidKernel(kernelIndex)) { return; }

	const auto *variable = FindGlobalVariable(name);
	if (!variable || variable->size != sizeof(uint32_t)) {
		Log::Error("ComputeShader \"{}\" に番号を入れる uint \"{}\" がありません", GetName(), name);
		return;
	}

	auto &bindings = kernels_[kernelIndex].bindings;
	std::erase_if(bindings, [&](const ResourceBinding &binding) { return binding.name == name; });
	if (!texture && !buffer) { return; }

	bindings.push_back({std::string(name), variable->offset, texture, buffer});
}


bool ou::ComputeShader::IsValidKernel(int kernelIndex) const {
	if (kernelIndex < 0 || kernelIndex >= static_cast<int>(kernels_.size())) {
		Log::Error("ComputeShader \"{}\" のカーネルの番号 {} は範囲外です", GetName(), kernelIndex);
		return false;
	}
	return true;
}
