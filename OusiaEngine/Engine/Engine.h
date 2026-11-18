#pragma once

#include "Diagnostics/Log.h"
#include "Editor/ImGuiManager.h"
#include "Editor/SceneRenderTarget.h"
#include "Graphics/DirectX12/GraphicsDevice_DX12.h"
#include "Graphics/GraphicsTypes.h"
#include "Platform/Window.h"
#include "Render/Asset/ShaderModule.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>


namespace ou {

// 前方宣言
class SceneView;


struct EngineDesc {
	std::string title;
	std::string productName;
	std::string projectDir;
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

	void Run();


private: // メンバ変数

	std::unique_ptr<LogSystem> logSystem_;
	std::unique_ptr<Window> window_;
	std::unique_ptr<Graphics::GraphicsDevice_DX12> device_;
	std::unique_ptr<ImGuiManager> imGuiManager_;

	Graphics::SwapChain swapChain_;
	SceneRenderTarget gameTarget_{"Game"};

	std::shared_ptr<ShaderModule> blitVs_;
	std::shared_ptr<ShaderModule> blitPs_;
	Graphics::PipelineState blitPso_;

	// フレーム
	std::chrono::steady_clock::time_point lastUpdateTime_;
	std::chrono::steady_clock::time_point reference_;


private: // メンバ関数

	void OpenFirstScene();

	bool Update();

	void Render();
	void RenderGame(const SceneRenderTarget &target, Graphics::CommandList cmd);
	void RenderSceneView(const SceneView &sceneView, Graphics::CommandList cmd);
	void BlitToSwapChain(const SceneRenderTarget &source, Graphics::CommandList cmd);

	void ResizeIfNeeded();

	void BindFullScreenViewport(Graphics::CommandList cmd);

	void WaitForFixedFrameRate();
};

} // namespace ou
