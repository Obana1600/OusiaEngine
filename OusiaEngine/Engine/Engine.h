#pragma once

#include "Asset/Shader.h"
#include "Core/Log.h"
#include "Graphics/GraphicsDevice_DX12.h"
#include "Graphics/GraphicsTypes.h"
#include "Graphics/TextureManager.h"
#include "Input/InputSystem.h"
#include "Platform/Window.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>


namespace ou {

struct EngineDesc {
	std::string title;
	uint32_t width = 1280;
	uint32_t height = 720;

#ifdef _DEBUG
	Graphics::ValidationMode validationMode = Graphics::ValidationMode::Enabled;
#else
	Graphics::ValidationMode validationMode = Graphics::ValidationMode::Disabled;
#endif
};


class Engine {
public: // メンバ関数

	void Initialize(const EngineDesc &desc);
	void Finalize();

	bool Update();

	Graphics::CommandList BeginDraw();

	void EndDraw(Graphics::CommandList cmd);

	const Graphics::RenderPassInfo &GetRenderPassInfo() const {
		return sceneInfo_;
	}


private: // メンバ変数

	std::unique_ptr<LogSystem> logSystem_;
	std::unique_ptr<Window> window_;
	std::unique_ptr<Graphics::GraphicsDevice_DX12> device_;

	Graphics::SwapChain swapChain_;

	// シーン描画
	Graphics::Texture sceneColor_;
	Graphics::Texture sceneDepth_;
	Graphics::RenderPassInfo sceneInfo_;

	std::shared_ptr<Shader> blitVs_;
	std::shared_ptr<Shader> blitPs_;
	Graphics::PipelineState blitPso_;

	// 固定 FPS
	std::chrono::steady_clock::time_point reference_;


private: // メンバ関数

	void CreateSceneTargets(uint32_t width, uint32_t height);

	void ResizeIfNeeded();

	void BindFullScreenViewport(Graphics::CommandList cmd);

	void WaitForFixedFrameRate();
};

} // namespace ou
