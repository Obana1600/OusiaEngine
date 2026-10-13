#include "ShaderImporter.h"
#include "Asset/Shader.h"
#include "Core/Log.h"
#include "Core/Paths.h"
#include "Graphics/GraphicsDevice.h"
#include "ShaderCompiler.h"

#include <string_view>
#include <utility>

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

// HLSL の拡張子からステージを決める
Graphics::ShaderStage InferStage(const std::filesystem::path &path) {
	// 接尾辞とステージのテーブル
	struct Entry {
		std::string_view suffix;
		Graphics::ShaderStage stage;
	};
	static constexpr Entry kTable[] = {
		{".MS", Graphics::ShaderStage::MS},
		{".AS", Graphics::ShaderStage::AS},
		{".VS", Graphics::ShaderStage::VS},
		{".HS", Graphics::ShaderStage::HS},
		{".DS", Graphics::ShaderStage::DS},
		{".GS", Graphics::ShaderStage::GS},
		{".PS", Graphics::ShaderStage::PS},
		{".CS", Graphics::ShaderStage::CS},
		{".LIB", Graphics::ShaderStage::LIB},
	};

	// 接尾辞が一致したステージを返す
	const auto suffix = ToUtf8(path.stem().extension());
	for (const auto &entry : kTable) {
		if (suffix == entry.suffix) {
			return entry.stage;
		}
	}
	return Graphics::ShaderStage::Count;
}

} // namespace



ou::ShaderImporter::ShaderImporter(std::filesystem::path assetPath)
	: AssetImporter(std::move(assetPath)),
	  stage_(InferStage(GetAssetPath())) {}


std::shared_ptr<Object> ou::ShaderImporter::Import() {
	return ImportShader();
}


std::shared_ptr<Shader> ou::ShaderImporter::ImportShader() {
	const auto path = ToUtf8(GetAssetPath());

	if (stage_ == Graphics::ShaderStage::Count) {
		Log::Error("シェーダステージが分かりません (ファイル名を X.VS.hlsl の形にするか SetStage で直接指定してください)");
		return nullptr;
	}

	// コンパイルに必要な設定
	ShaderCompiler::CompilerInput input{};
	input.flags = kCompileFlags;
	input.stage = stage_;
	input.minShaderModel = minShaderModel_;
	input.shaderPath = path;
	input.defines = defines_;

	// シェーダのコンパイル
	const auto output = ShaderCompiler::Compile(input);
	if (!output.IsValid()) {
		Log::Error("シェーダのコンパイルに失敗しました: {}\n{}", path, output.errorMessage);
		return nullptr;
	}
	if (!output.errorMessage.empty()) {
		Log::Warning("シェーダの警告: {}\n{}", path, output.errorMessage);
	}

	// グラフィクス層のシェーダを作成
	Graphics::Shader graphicsShader{};
	if (!Graphics::GetDevice()->CreateShader(stage_, output.shaderData, output.shaderSize, &graphicsShader)) {
		Log::Error("シェーダの作成に失敗しました");
		return nullptr;
	}

	// オブジェクト層のシェーダを作成
	auto shader = std::make_shared<Shader>(graphicsShader);
	shader->SetName(GetName());
	return shader;
}


void ou::ShaderImporter::SetStage(Graphics::ShaderStage stage) {
	stage_ = stage;
}


void ou::ShaderImporter::SetMinShaderModel(Graphics::ShaderModel model) {
	minShaderModel_ = model;
}


void ou::ShaderImporter::SetDefines(std::vector<std::string> defines) {
	defines_ = std::move(defines);
}
