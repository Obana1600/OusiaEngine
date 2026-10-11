#pragma once

#include "Core/Log.h"
#include "Graphics/DXCompiler.h"
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
	ValidationMode validationMode = ValidationMode::Enabled;
#else
	ValidationMode validationMode = ValidationMode::Disabled;
#endif
};


class Engine {
public: // メンバ関数

	void Initialize(const EngineDesc &desc);
	void Finalize();

	bool Update();

	CommandList BeginDraw();

	void EndDraw(CommandList cmd);

	const RenderPassInfo &GetRenderPassInfo() const {
		return sceneInfo_;
	}


private: // メンバ変数

	std::unique_ptr<LogSystem> logSystem_;
	std::unique_ptr<Window> window_;
	std::unique_ptr<GraphicsDevice_DX12> device_;

	SwapChain swapChain_;

	// シーン描画
	Texture sceneColor_;
	Texture sceneDepth_;
	RenderPassInfo sceneInfo_;

	Shader blitVs_;
	Shader blitPs_;
	PipelineState blitPso_;

	// 固定 FPS
	std::chrono::steady_clock::time_point reference_;


private: // メンバ関数

	void CreateSceneTargets(uint32_t width, uint32_t height);

	void ResizeIfNeeded();

	void BindFullScreenViewport(CommandList cmd);

	void WaitForFixedFrameRate();
};

} // namespace ou
