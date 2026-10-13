#include "Engine.h"
#include "2D/Sprite.h"
#include "Core/Fatal.h"
#include "Core/Paths.h"
#include "Graphics/DXCompiler.h"
#include "Render/Material.h"
#include "Render/ModelManager.h"

#include <format>
#include <thread>

#pragma comment(lib, "winmm.lib")

using namespace ou;


namespace {

// クリアカラー
constexpr float kClearColor[4] = {0.1f, 0.25f, 0.5f, 1.0f};

// 1/60 秒ピッタリの時間
const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
// 1/60 秒よりわずかに短い時間
const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

struct BlitConstants {
	uint32_t sourceIndex;
};

} // namespace



void ou::Engine::Initialize(const EngineDesc &desc) {
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	logSystem_ = std::make_unique<LogSystem>(LogDirectory("CG2_DirectXGame"), std::span<const std::string>());

	window_ = Window::Create({.title = desc.title, .width = desc.width, .height = desc.height});

	// デバイス
	device_ = std::make_unique<Graphics::GraphicsDevice_DX12>(desc.validationMode);
	Graphics::GetDevice() = device_.get();

	// スワップチェーン
	Graphics::SwapChainDesc scDesc{};
	scDesc.width = window_->ClientWidth();
	scDesc.height = window_->ClientHeight();
	scDesc.bufferCount = 2;
	scDesc.format = Graphics::Format::B8G8R8A8_UNORM_SRGB;
	for (int i = 0; i < 4; i++) {
		scDesc.clearColor[i] = kClearColor[i];
	}
	if (!device_->CreateSwapChain(&scDesc, window_->Hwnd(), &swapChain_)) {
		FatalError("スワップチェーンの作成に失敗しました。");
	}

	CreateSceneTargets(scDesc.width, scDesc.height);

	// blit
	DXCompiler::GetInstance()->Initialize();
	blitVs_ = DXCompiler::GetInstance()->CompileEngineShader(L"Blit.VS.hlsl", Graphics::ShaderStage::VS);
	blitPs_ = DXCompiler::GetInstance()->CompileEngineShader(L"Blit.PS.hlsl", Graphics::ShaderStage::PS);

	Graphics::PipelineStateDesc blitPsoDesc{};
	blitPsoDesc.vs = &blitVs_;
	blitPsoDesc.ps = &blitPs_;
	const auto scInfo = Graphics::RenderPassInfo::From(swapChain_.GetDesc());
	if (!device_->CreatePipelineState(&blitPsoDesc, &blitPso_, &scInfo)) {
		FatalError("Blit の PSO の作成に失敗しました。");
	}

	// スプライト
	Sprite::StaticInitialize(sceneInfo_, scDesc.width, scDesc.height);

	// マテリアル
	Material::StaticInitialize(sceneInfo_);

	// その他
	InputSystem::GetInstance()->Initialize(window_->Hwnd());

	// 固定 FPS
	reference_ = std::chrono::steady_clock::now();
	timeBeginPeriod(1);
}


void ou::Engine::Finalize() {
	device_->WaitForGpu();

	TextureManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();
	Sprite::StaticFinalize();
	Material::StaticFinalize();

	timeEndPeriod(1);

	blitPso_ = {};
	blitPs_ = {};
	blitVs_ = {};
	sceneDepth_ = {};
	sceneColor_ = {};
	swapChain_ = {};

	Graphics::GetDevice() = nullptr;
	device_.reset();
	window_.reset();
	logSystem_.reset();

	CoUninitialize();
}


bool ou::Engine::Update() {
	window_->PumpMessages();
	InputSystem::GetInstance()->Update();
	return window_->IsCloseRequested();
}


Graphics::CommandList ou::Engine::BeginDraw() {
	// 最小化中は描かない
	if (window_->IsMinimized()) {
		Sleep(10);
		return {};
	}

	ResizeIfNeeded();

	auto cmd = device_->BeginCommandList();

	const Graphics::RenderPassImage images[] = {
		Graphics::RenderPassImage::RenderTarget(&sceneColor_, Graphics::RenderPassImage::LoadOp::CLEAR),
		Graphics::RenderPassImage::DepthStencil(&sceneDepth_, Graphics::RenderPassImage::LoadOp::CLEAR, Graphics::RenderPassImage::StoreOp::DONTCARE),
	};
	device_->RenderPassBegin(images, 2, cmd);
	BindFullScreenViewport(cmd);

	return cmd;
}


void ou::Engine::EndDraw(Graphics::CommandList cmd) {
	device_->RenderPassEnd(cmd);

	device_->RenderPassBegin(&swapChain_, cmd);
	{
		BindFullScreenViewport(cmd);
		device_->BindPipelineState(&blitPso_, cmd);

		BlitConstants constants{};
		constants.sourceIndex = static_cast<uint32_t>(device_->GetDescriptorIndex(&sceneColor_, Graphics::ViewType::SRV));
		device_->BindDynamicConstantBuffer(constants, 0, cmd);

		device_->Draw(3, 0, cmd);
	}
	device_->RenderPassEnd(cmd);

	device_->SubmitCommandLists();

	WaitForFixedFrameRate();
}


void ou::Engine::CreateSceneTargets(uint32_t width, uint32_t height) {
	Graphics::TextureDesc colDesc{};
	colDesc.width = width;
	colDesc.height = height;
	colDesc.format = Graphics::Format::B8G8R8A8_UNORM_SRGB;
	colDesc.bindFlags = Graphics::BindFlag::RENDER_TARGET | Graphics::BindFlag::SHADER_RESOURCE;
	colDesc.layout = Graphics::ResourceState::SHADER_RESOURCE;
	for (int i = 0; i < 4; i++) {
		colDesc.clear.color[i] = kClearColor[i];
	}
	if (!device_->CreateTexture(&colDesc, nullptr, &sceneColor_)) {
		FatalError("SceneColor の作成に失敗しました。");
	}
	device_->SetName(&sceneColor_, "SceneColor");

	Graphics::TextureDesc depthDesc{};
	depthDesc.width = width;
	depthDesc.height = height;
	depthDesc.format = Graphics::Format::D32_FLOAT;
	depthDesc.bindFlags = Graphics::BindFlag::DEPTH_STENCIL;
	depthDesc.layout = Graphics::ResourceState::DEPTHSTENCIL;
	depthDesc.clear.depthStencil.depth = 1.0f;
	if (!device_->CreateTexture(&depthDesc, nullptr, &sceneDepth_)) {
		FatalError("SceneDepth の作成に失敗しました。");
	}
	device_->SetName(&sceneDepth_, "SceneDepth");

	const Graphics::RenderPassImage images[] = {
		Graphics::RenderPassImage::RenderTarget(&sceneColor_),
		Graphics::RenderPassImage::DepthStencil(&sceneDepth_),
	};
	sceneInfo_ = Graphics::RenderPassInfo::From(images, 2);
}


void ou::Engine::ResizeIfNeeded() {
	const auto &current = swapChain_.GetDesc();
	if (window_->ClientWidth() == current.width && window_->ClientHeight() == current.height) {
		return;
	}

	auto desc = current;
	desc.width = window_->ClientWidth();
	desc.height = window_->ClientHeight();
	device_->CreateSwapChain(&desc, window_->Hwnd(), &swapChain_);

	CreateSceneTargets(desc.width, desc.height);
}


void ou::Engine::BindFullScreenViewport(Graphics::CommandList cmd) {
	const auto &desc = swapChain_.GetDesc();

	Graphics::Viewport viewport{};
	viewport.width = static_cast<float>(desc.width);
	viewport.height = static_cast<float>(desc.height);
	device_->BindViewports(1, &viewport, cmd);

	Graphics::Scissor scissor{};
	scissor.right = static_cast<int32_t>(desc.width);
	scissor.bottom = static_cast<int32_t>(desc.height);
	device_->BindScissorRects(1, &scissor, cmd);
}


void ou::Engine::WaitForFixedFrameRate() {
	const auto now = std::chrono::steady_clock::now();
	const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	if (elapsed < kMinCheckTime) {
		while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	reference_ = std::chrono::steady_clock::now();
}
