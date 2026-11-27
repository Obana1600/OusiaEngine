#include "ComputeShaderImporter.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/ShaderCompiler.h"
#include "Platform/Paths.h"
#include "Render/Asset/ComputeShader.h"

#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>
#include <utility>
#include <vector>

using namespace ou;


namespace {

// ビルド構成ごとのコンパイル設定
constexpr ShaderCompiler::Flags kCompileFlags =
#ifdef _DEBUG
	ShaderCompiler::Flags::DISABLE_OPTIMIZATION | ShaderCompiler::Flags::EMBED_DEBUG_INFO;
#elif _DEVELOP
	ShaderCompiler::Flags::EMBED_DEBUG_INFO;
#elif _RELEASE
	ShaderCompiler::Flags::NONE;
#endif

// "#pragma kernel ○○" を書いてある順に集める
std::vector<std::string> FindKernelNames(const std::filesystem::path &path) {
	std::ifstream file(path, std::ios::binary);
	std::stringstream text;
	text << file.rdbuf();

	static const std::regex kPragmaKernel(R"(^\s*#\s*pragma\s+kernel\s+([A-Za-z_]\w*))");
	std::vector<std::string> names;
	std::string line;
	while (std::getline(text, line)) {
		std::smatch match;
		if (std::regex_search(line, match, kPragmaKernel)) {
			names.push_back(match[1].str());
		}
	}
	return names;
}

} // namespace



ou::ComputeShaderImporter::ComputeShaderImporter(std::string assetPath, std::filesystem::path fullPath)
	: AssetImporter(std::move(assetPath), std::move(fullPath)) {}


std::shared_ptr<Object> ou::ComputeShaderImporter::Import() {
	return ImportComputeShader();
}


std::shared_ptr<ComputeShader> ou::ComputeShaderImporter::ImportComputeShader() {
	const auto path = ToUtf8(GetFullPath());

	const auto kernelNames = FindKernelNames(GetFullPath());
	if (kernelNames.empty()) {
		Log::Error("ComputeShader に #pragma kernel がありません: {}", path);
		return nullptr;
	}

	auto computeShader = std::make_shared<ComputeShader>();
	computeShader->SetName(GetName());

	for (const auto &kernelName : kernelNames) {
		ShaderCompiler::CompilerInput input{};
		input.flags = kCompileFlags;
		input.stage = Graphics::ShaderStage::CS;
		input.shaderPath = path;
		input.entryPoint = kernelName;
		if (const auto engineAssets = FindEngineAssetsFolder()) {
			input.includeDirectories.push_back(ToUtf8(*engineAssets / L"Shader"));
		}

		const auto output = ShaderCompiler::Compile(input);
		if (!output.IsValid()) {
			Log::Error("ComputeShader のカーネル \"{}\" のコンパイルに失敗しました: {}\n{}", kernelName, path, output.errorMessage);
			return nullptr;
		}
		if (!output.errorMessage.empty()) {
			Log::Warning("ComputeShader のカーネル \"{}\" の警告: {}\n{}", kernelName, path, output.errorMessage);
		}

		ComputeShader::Kernel kernel;
		kernel.name = kernelName;
		const auto &groupSize = output.reflection.threadGroupSize;
		kernel.threadGroupSize = {groupSize[0], groupSize[1], groupSize[2]};
		if (!Graphics::GetDevice()->CreateShader(Graphics::ShaderStage::CS, output.shaderData, output.shaderSize, &kernel.shader)) {
			Log::Error("ComputeShader のカーネル \"{}\" の作成に失敗しました: {}", kernelName, path);
			return nullptr;
		}
		computeShader->kernels_.push_back(std::move(kernel));

		for (const auto &variable : output.reflection.globals) {
			if (!computeShader->FindGlobalVariable(variable.name)) {
				computeShader->globalVariables_.push_back({variable.name, variable.offset, variable.size});
			}
		}
		const auto globalsSize = std::max<size_t>(computeShader->globals_.size(), output.reflection.globalsSize);
		computeShader->globals_.resize(globalsSize);
	}

	return computeShader;
}
