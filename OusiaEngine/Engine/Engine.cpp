#include "Engine.h"
#include "Asset/AssetDatabase.h"
#include "Audio/XAudio2/Audio.h"
#include "Core/Application.h"
#include "Core/StringUtil.h"
#include "Core/Time.h"
#include "Diagnostics/Fatal.h"
#include "Input/Input.h"
#include "Particle/ParticleSystemRenderer.h"
#include "Physics/Jolt/PhysicsWorld.h"
#include "Platform/Paths.h"
#include "Platform/Screen.h"
#include "Render/Asset/Material.h"
#include "Render/LineRenderer.h"
#include "Scene/SceneManager.h"
#include "UI/Graphic.h"
#include "UI/Text.h"

#include <filesystem>
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

	// プロジェクトのフォルダ
	SetProjectDirectory(std::filesystem::path(Utf8ToWide(desc.projectDir)));

	// ログ
	const auto productName = desc.productName.empty() ? "OusiaEngine_Project" : desc.productName;
	logSystem_ = std::make_unique<LogSystem>(LogDirectory(productName), std::span<const std::string>());

#ifdef WITH_EDITOR
	if (!FindAssetsFolder()) {
		Log::Error("Assets フォルダが見つかりません");
	}
#endif

	// オーディオ
	if (!Audio::Initialize()) {
		Log::Error("オーディオを初期化できませんでした。音は鳴りません");
	}

	// 物理
	PhysicsWorld::Initialize();

	// ウィンドウ
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

	Screen::SetSize(scDesc.width, scDesc.height);
	CreateSceneTargets(scDesc.width, scDesc.height);

	// blit
	blitVs_ = AssetDatabase::LoadAssetAtPath<ShaderModule>("EngineAssets/Shader/Blit.VS.hlsl");
	blitPs_ = AssetDatabase::LoadAssetAtPath<ShaderModule>("EngineAssets/Shader/Blit.PS.hlsl");
	if (!blitVs_ || !blitPs_) {
		FatalError("Blit のシェーダの作成に失敗しました");
	}

	Graphics::PipelineStateDesc blitPsoDesc{};
	blitPsoDesc.vs = &blitVs_->GetGraphicsShader();
	blitPsoDesc.ps = &blitPs_->GetGraphicsShader();
	const auto scInfo = Graphics::RenderPassInfo::From(swapChain_.GetDesc());
	if (!device_->CreatePipelineState(&blitPsoDesc, &blitPso_, &scInfo)) {
		FatalError("Blit の PSO の作成に失敗しました。");
	}

	// imgui
	imGuiManager_ = std::make_unique<ImGuiManager>(window_.get());

	// 初期化
	Material::StaticInitialize();
	Graphic::StaticInitialize();
	Text::StaticInitialize();
	ParticleSystemRenderer::StaticInitialize();
	LineRenderer::StaticInitialize();

	// 入力
	if (!Input::Initialize(window_.get())) {
		Log::Error("入力の初期化に失敗しました");
	}

	// 固定 FPS
	reference_ = std::chrono::steady_clock::now();
	timeBeginPeriod(1);

	// フレーム計測開始
	lastUpdateTime_ = std::chrono::steady_clock::now();
}


void ou::Engine::Finalize() {
	device_->WaitForGpu();

	imGuiManager_.reset();
	SceneManager::Finalize();

	PhysicsWorld::Finalize();

	Material::StaticFinalize();
	Graphic::StaticFinalize();
	Text::StaticFinalize();
	ParticleSystemRenderer::StaticFinalize();
	LineRenderer::StaticFinalize();

	AssetDatabase::UnloadAllAssets();

	Audio::Finalize();

	Input::Finalize();

	timeEndPeriod(1);

	blitPso_ = {};
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
	// Time を進める
	const auto now = std::chrono::steady_clock::now();
	const auto duration = std::chrono::duration<float>(now - lastUpdateTime_).count();
	Time::Advance(duration);
	lastUpdateTime_ = now;

	window_->PumpMessages();
	Input::Update();
	imGuiManager_->NewFrame();
	return window_->IsCloseRequested() || Application::IsQuitRequested();
}


Graphics::CommandList ou::Engine::BeginDraw() {
	imGuiManager_->EndFrame();

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


void ou::Engine::BeginOverlay(Graphics::CommandList cmd) {
	device_->RenderPassEnd(cmd);

	device_->RenderPassBegin(&swapChain_, cmd);
	BindFullScreenViewport(cmd);
	device_->BindPipelineState(&blitPso_, cmd);

	BlitConstants constants{};
	constants.sourceIndex = static_cast<uint32_t>(device_->GetDescriptorIndex(&sceneColor_, Graphics::ViewType::SRV));
	device_->BindDynamicConstantBuffer(constants, 0, cmd);

	device_->Draw(3, 0, cmd);

	isOverlayBegun_ = true;
}


void ou::Engine::EndDraw(Graphics::CommandList cmd) {
	if (!isOverlayBegun_) {
		BeginOverlay(cmd);
	}
	imGuiManager_->Render(cmd);
	device_->RenderPassEnd(cmd);
	isOverlayBegun_ = false;

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

	Screen::SetSize(desc.width, desc.height);
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
