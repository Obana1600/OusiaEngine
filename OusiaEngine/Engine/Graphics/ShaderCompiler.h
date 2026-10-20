#pragma once

#include "Types/GraphicsEnums.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


namespace ou::ShaderCompiler {

enum class Flags : uint32_t {
	NONE = 0,
	DISABLE_OPTIMIZATION = 1 << 0, // -Od
	EMBED_DEBUG_INFO = 1 << 1,	   // -Zi -Qembed_debug
};

struct CompilerInput {
	Flags flags = Flags::NONE;
	Graphics::ShaderStage stage = Graphics::ShaderStage::Count;
	Graphics::ShaderModel minShaderModel = Graphics::ShaderModel::SM_6_6;
	std::string shaderPath; // UTF-8 に変換して注入
	std::string entryPoint = "main";
	std::vector<std::string> includeDirectories;
	std::vector<std::string> defines;
};

struct CompilerOutput {
	std::shared_ptr<void> internalState;
	const uint8_t *shaderData = nullptr;
	size_t shaderSize = 0;
	std::string errorMessage;

	bool IsValid() const {
		return internalState != nullptr;
	}
};

CompilerOutput Compile(const CompilerInput &input);

} // namespace ou::ShaderCompiler


// ビット演算の許可
template <>
struct EnableBitmaskOperators<ou::ShaderCompiler::Flags> {
	static constexpr bool enable = true;
};
