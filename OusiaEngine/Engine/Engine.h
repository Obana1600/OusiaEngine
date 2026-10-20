#pragma once

#include "Diagnostics/Log.h"
#include "Editor/ImGuiManager.h"
#include "Graphics/DirectX12/GraphicsDevice_DX12.h"
#include "Graphics/GraphicsTypes.h"
#include "Input/InputSystem.h"
#include "Platform/Window.h"
#include "Render/Asset/ShaderModule.h"

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

	void BeginOverlay(Graphics::CommandList cmd);

	void EndDraw(Graphics::CommandList cmd);

	const Graphics::RenderPassInfo &GetRenderPassInfo() const {
		return sceneInfo_;
	}


private: // メンバ変数

	std::unique_ptr<LogSystem> logSystem_;
	std::unique_ptr<Window> window_;
	std::unique_ptr<Graphics::GraphicsDevice_DX12> device_;
	std::unique_ptr<ImGuiManager> imGuiManager_;

	Graphics::SwapChain swapChain_;

	// シーン描画
	Graphics::Texture sceneColor_;
	Graphics::Texture sceneDepth_;
	Graphics::RenderPassInfo sceneInfo_;

	std::shared_ptr<ShaderModule> blitVs_;
	std::shared_ptr<ShaderModule> blitPs_;
	Graphics::PipelineState blitPso_;

	// フレーム
	std::chrono::steady_clock::time_point lastUpdateTime_;
	std::chrono::steady_clock::time_point reference_;

	bool isOverlayBegun_ = false;


private: // メンバ関数

	void CreateSceneTargets(uint32_t width, uint32_t height);

	void ResizeIfNeeded();

	void BindFullScreenViewport(Graphics::CommandList cmd);

	void WaitForFixedFrameRate();
};

} // namespace ou
