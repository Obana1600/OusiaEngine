#include "Engine.h"
#include "Asset/AssetDatabase.h"
#include "Audio/XAudio2/Audio.h"
#include "Core/Application.h"
#include "Core/PlayerSettings.h"
#include "Core/StringUtil.h"
#include "Core/Time.h"
#include "Core/TimeManager.h"
#include "Diagnostics/Fatal.h"
#include "Input/Cursor.h"
#include "Input/Input.h"
#include "Particle/ParticleSystemRenderer.h"
#include "Physics/Box3D/PhysicsWorld.h"
#include "Physics/Physics.h"
#include "Platform/Paths.h"
#include "Platform/Screen.h"
#include "PostProcess/PostProcessPass.h"
#include "PostProcess/VolumeManager.h"
#include "Render/Asset/Material.h"
#include "Render/CopyColorPass.h"
#include "Render/CopyDepthPass.h"
#include "Render/GraphicsSettings.h"
#include "Render/LineRenderer.h"
#include "Scene/LayerMask.h"
#include "Scene/SceneManager.h"
#include "Scene/TagManager.h"
#include "Render/ImmediateCommands.h"
#include "Render/PerFrame.h"
#include "Script/ScriptEngine.h"
#include "Serialize/EngineTypes.h"
#include "UI/Graphic.h"
#include "UI/Text.h"
#ifdef WITH_EDITOR
#include "Editor/EditorApplication.h"
#include "Editor/EditorMainWindow.h"
#include "Editor/EditorPrefs.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/EditorWindow.h"
#include "Editor/GameView.h"
#include "Editor/GizmoRenderer.h"
#include "Editor/LogEntries.h"
#include "Editor/ParticleEffectPreview.h"
#include "Editor/SceneView.h"
#include "Editor/Undo.h"
#endif

#include <filesystem>

#include <magic_enum/magic_enum.hpp>
#include <thread>
#ifdef WITH_EDITOR
#include <shellapi.h>
#endif

#pragma comment(lib, "winmm.lib")
#ifdef WITH_EDITOR
#pragma comment(lib, "Shell32.lib")
#endif


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

#ifdef WITH_EDITOR
// 起動引数の -projectPath <フォルダ>（Unity と同じ）。ビルド時の PROJECT_DIR 以外のプロジェクト（Samples/ など）を開く。
// WinMain の lpCmdLine は ANSI なので、日本語のパスでも壊れない GetCommandLineW から読む
std::optional<std::filesystem::path> FindProjectPathArgument() {
	int argc = 0;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (!argv) { return std::nullopt; }

	std::optional<std::filesystem::path> path;
	for (int i = 1; i + 1 < argc; i++) {
		if (_wcsicmp(argv[i], L"-projectPath") == 0) {
			path = std::filesystem::absolute(argv[i + 1]);
			break;
		}
	}
	LocalFree(argv);
	return path;
}

// エディタのウィンドウの状態を覚える EditorPrefs のキー（PC ごと。Unity も閉じたときの状態に戻す）
constexpr const char *kEditorWindowModeKey = "EditorWindow.Mode";
constexpr const char *kEditorWindowRectKeys[] = {"EditorWindow.Left", "EditorWindow.Top", "EditorWindow.Right", "EditorWindow.Bottom"};

// 前回閉じたときの状態。初めては最大化
WindowDesc MakeEditorWindowDesc(const std::string &title, uint32_t width, uint32_t height) {
	WindowDesc desc{.title = title, .width = width, .height = height, .fullScreenMode = FullScreenMode::MaximizedWindow};

	desc.fullScreenMode = magic_enum::enum_cast<FullScreenMode>(EditorPrefs::GetString(kEditorWindowModeKey)).value_or(desc.fullScreenMode);

	if (EditorPrefs::HasKey(kEditorWindowRectKeys[0])) {
		RECT rect{
			EditorPrefs::GetInt(kEditorWindowRectKeys[0]), EditorPrefs::GetInt(kEditorWindowRectKeys[1]),
			EditorPrefs::GetInt(kEditorWindowRectKeys[2]), EditorPrefs::GetInt(kEditorWindowRectKeys[3])};
		if (rect.right > rect.left && rect.bottom > rect.top) { desc.windowedRect = rect; }
	}
	return desc;
}

void SaveEditorWindow(const Window &window) {
	EditorPrefs::SetString(kEditorWindowModeKey, magic_enum::enum_name(window.GetFullScreenMode()));

	const auto rect = window.GetWindowedRect();
	const LONG values[] = {rect.left, rect.top, rect.right, rect.bottom};
	for (int i = 0; i < 4; i++) {
		EditorPrefs::SetInt(kEditorWindowRectKeys[i], static_cast<int>(values[i]));
	}
}
#else
// Alt+Enter で全画面とウィンドウを切り替える（Unity の Player と同じ）
std::optional<LRESULT> ToggleFullScreenOnAltEnter(HWND, UINT msg, WPARAM wparam, LPARAM lparam) {
	constexpr LPARAM kAltDown = 1 << 29;
	constexpr LPARAM kRepeat = 1 << 30;

	if (msg == WM_SYSKEYDOWN && wparam == VK_RETURN && (lparam & kAltDown)) {
		if (!(lparam & kRepeat)) { Screen::SetFullScreen(!Screen::IsFullScreen()); }
		return 0;
	}
	// Alt+Enter の文字は捨てる（そのままだと警告音が鳴る）
	if (msg == WM_SYSCHAR && wparam == VK_RETURN) { return 0; }
	return std::nullopt;
}
#endif

} // namespace



void ou::Engine::Initialize(const EngineDesc &desc) {
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	// プロジェクトのフォルダ
#ifdef WITH_EDITOR
	const auto distributedProject = ExecutableDirectory().parent_path();
	if (const auto projectPath = FindProjectPathArgument()) {
		SetProjectDirectory(*projectPath);
	} else if (std::filesystem::is_directory(distributedProject / L"Assets")) {
		SetProjectDirectory(distributedProject);
	} else {
		SetProjectDirectory(std::filesystem::path(Utf8ToWide(desc.projectDir)));
	}
#else
	SetProjectDirectory(std::filesystem::path(Utf8ToWide(desc.projectDir)));
#endif

	// アプリ名
	PlayerSettings::Load(desc.productName);
	const auto &productName = PlayerSettings::GetProductName();

	// ログ（コンソールは起動時のログも拾えるように先に）
#ifdef WITH_EDITOR
	LogEntries::Initialize();
#endif
	logSystem_ = std::make_unique<LogSystem>(LogDirectory(productName), std::span<const std::string>());

#ifdef WITH_EDITOR
	if (!FindAssetsFolder()) {
		Log::Error("Assets フォルダが見つかりません");
	}
#endif

	// アセットの読み込み
	AssetDatabase::Refresh();

	// レイヤー
	LayerMask::LoadLayerNames();
	TagManager::Load();

	// Build Settings
	SceneManager::LoadBuildSettings();

	// スクリプト
	ScriptEngine::Initialize();

	// オーディオ
	if (!Audio::Initialize()) {
		Log::Error("オーディオを初期化できませんでした。音は鳴りません");
	}

	// 物理
	PhysicsWorld::Initialize();
	Physics::LoadDynamicsManager();

	// 時間（Project Settings の Time）
	TimeManager::Load();

	// ウィンドウ
#ifdef WITH_EDITOR
	window_ = Window::Create(MakeEditorWindowDesc(productName, desc.width, desc.height));
#else
	window_ = Window::Create({.title = productName,
							  .width = PlayerSettings::GetDefaultScreenWidth(),
							  .height = PlayerSettings::GetDefaultScreenHeight(),
							  .fullScreenMode = PlayerSettings::GetFullScreenMode()});
	window_->AddMessageHandler(ToggleFullScreenOnAltEnter);
#endif
	Screen::sWindow_ = window_.get();

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

	// 組み込みコンポーネントのレジスタ
	RegisterEngineTypes();

	// 初期化
	Material::StaticInitialize();
	Graphic::StaticInitialize();
	Text::StaticInitialize();
	ParticleSystemRenderer::StaticInitialize();
	LineRenderer::StaticInitialize();
#ifdef WITH_EDITOR
	GizmoRenderer::StaticInitialize();
#endif

	// ポストプロセス
	GraphicsSettings::Load();
	PostProcessPass::StaticInitialize();
	CopyDepthPass::StaticInitialize();
	CopyColorPass::StaticInitialize();

	// 入力
	if (!Input::Initialize(window_.get())) {
		Log::Error("入力の初期化に失敗しました");
	}
	Cursor::Initialize(window_.get());

	// 固定 FPS
	reference_ = std::chrono::steady_clock::now();
	timeBeginPeriod(1);

	// フレーム計測開始
	lastUpdateTime_ = std::chrono::steady_clock::now();
}


void ou::Engine::Finalize() {
	device_->WaitForGpu();

#ifdef WITH_EDITOR
	EditorWindow::DestroyAll();
	GizmoRenderer::StaticFinalize();
#endif

	imGuiManager_.reset();
	SceneManager::Finalize();
	GraphicsSettings::Unload();
	AssetDatabase::UnloadAllAssets();
	ScriptEngine::Finalize();

	PhysicsWorld::Finalize();

	Material::StaticFinalize();
	Graphic::StaticFinalize();
	Text::StaticFinalize();
	ParticleSystemRenderer::StaticFinalize();
	LineRenderer::StaticFinalize();

	// ポストプロセス
	PostProcessPass::StaticFinalize();
	CopyDepthPass::StaticFinalize();
	CopyColorPass::StaticFinalize();

	// オーディオ
	Audio::Finalize();

	// 入力
	Cursor::Finalize();
	Input::Finalize();

	timeEndPeriod(1);

	blitPso_ = {};
	gameTarget_.Release();
	swapChain_ = {};

	Graphics::GetDevice() = nullptr;
	device_.reset();
#ifdef WITH_EDITOR
	SaveEditorWindow(*window_);
#endif
	Screen::sWindow_ = nullptr;
	window_.reset();
	logSystem_.reset();
#ifdef WITH_EDITOR
	LogEntries::Finalize();
#endif

	CoUninitialize();
}


void ou::Engine::Run() {
	OpenFirstScene();

	while (!Update()) {
		SceneManager::Update();
		Render();
	}

	// OnDisable・OnDestroy は、このあと Finalize でシーンを消すときに呼ぶ
	SceneManager::NotifyApplicationQuit();
}


void ou::Engine::OpenFirstScene() {
#ifdef WITH_EDITOR
	EditorSceneManager::OpenLastScene();
#else
	if (SceneManager::GetSceneCountInBuildSettings() == 0) {
		Log::Error("Build Settings にシーンがありません (ProjectSettings/EditorBuildSettings.json)");
		return;
	}
	ScriptEngine::InvokeBeforeSceneLoadMethods();
	SceneManager::LoadScene(0);
	// AfterSceneLoad は最初のシーンの Awake のあとに呼ぶので、次のフレームを待たずにここで読み込む
	SceneManager::LoadPendingScene();
	ScriptEngine::InvokeAfterSceneLoadMethods();
#endif
}


// TODO: ぐちゃぐちゃスギ
bool ou::Engine::Update() {
	const auto now = std::chrono::steady_clock::now();
	const auto duration = std::chrono::duration<float>(now - lastUpdateTime_).count();
#ifdef WITH_EDITOR
	// Pause・Step で、時間の進め方が変わる
	EditorApplication::BeginGameFrame(duration);
#else
	Time::Advance(duration);
#endif
	lastUpdateTime_ = now;

	window_->PumpMessages();
#ifdef WITH_EDITOR
	// エディタは閉じる前に保存していない変更を聞く。キャンセルできるように、閉じる要求をいったん取り消す
	if (window_->IsCloseRequested()) {
		window_->CancelClose();
		EditorMainWindow::RequestQuit();
	}
#endif
#ifdef WITH_EDITOR
	const auto *gameView = GameView::FindVisible();
	Input::SetGameView(gameView ? gameView->GetImageOrigin() : Vector2::Zero,
					   gameView && gameView->HasFocus(),
					   gameView && gameView->IsImageHovered());
	Cursor::SetGameView(gameView ? gameView->GetImageOrigin() : Vector2::Zero,
						gameView && gameView->HasFocus(),
						gameView && gameView->IsImageHovered());
#endif
	Input::Update();
#ifdef WITH_EDITOR
	EditorApplication::ApplyPendingPlayModeChange();
	EditorApplication::ReloadScriptsIfChanged();
	Undo::ApplyPendingRequest();
#endif
	imGuiManager_->NewFrame();
#ifdef WITH_EDITOR
	LogEntries::Update();
	EditorMainWindow::Draw();
	ParticleEffectPreview::Update();
#endif
	Cursor::Update();
#ifdef WITH_EDITOR
	return EditorMainWindow::IsQuitApproved();
#else
	return window_->IsCloseRequested() || Application::IsQuitRequested();
#endif
}


void ou::Engine::Render() {
	imGuiManager_->EndFrame();

	// 最小化中は描かない
	if (window_->IsMinimized()) {
		Sleep(10);
		return;
	}

	ResizeIfNeeded();

	auto cmd = device_->BeginCommandList();
	BindPerFrame(cmd);

#ifdef WITH_EDITOR
	// エディタではゲームを Game ビューに、エディタのカメラで Scene ビューに描き、スワップチェーンには ImGui だけを描く
	if (const auto *gameView = GameView::FindVisible()) {
		RenderGame(gameView->GetRenderTarget(), cmd);
	}
	if (const auto *sceneView = SceneView::FindVisible()) {
		RenderSceneView(*sceneView, cmd);
	}
	device_->RenderPassBegin(&swapChain_, cmd);
#else
	const auto &desc = swapChain_.GetDesc();
	gameTarget_.Resize(desc.width, desc.height);
	RenderGame(gameTarget_, cmd);

	device_->RenderPassBegin(&swapChain_, cmd);
	BlitToSwapChain(gameTarget_, cmd);
#endif

	imGuiManager_->Render(cmd);
	device_->RenderPassEnd(cmd);

	device_->SubmitCommandLists();

	ImmediateCommands::OnSubmitted();

	WaitForFixedFrameRate();
}


void ou::Engine::BindPerFrame(Graphics::CommandList cmd) {
	PerFrame perFrame{};

	const auto time = Time::GetTime();
	perFrame.time = {time / 20.0f, time, time * 2.0f, time * 3.0f};

	SceneManager::CollectLights(perFrame);
	device_->BindDynamicConstantBuffer(perFrame, 3, cmd);
}


void ou::Engine::FillTargetInfo(Camera::PerCamera &perCamera, const SceneRenderTarget &target) const {
	const auto width = static_cast<float>(target.GetWidth());
	const auto height = static_cast<float>(target.GetHeight());
	perCamera.screenSize = {width, height, 1.0f / width, 1.0f / height};

	perCamera.depthTextureIndex = static_cast<uint32_t>(device_->GetDescriptorIndex(&target.GetDepthCopy(), Graphics::ViewType::SRV));
	perCamera.opaqueTextureIndex = static_cast<uint32_t>(device_->GetDescriptorIndex(&target.GetOpaqueTexture(), Graphics::ViewType::SRV));
}


void ou::Engine::RenderScene(const SceneRenderTarget &target, const Camera::PerCamera &perCamera, Graphics::CommandList cmd) {
	target.BeginScenePass(cmd);
	SceneManager::RenderWithCamera(perCamera, RenderQueueRange::Opaque, cmd);
	device_->RenderPassEnd(cmd);

	CopyDepthPass::Render(target, cmd);
	CopyColorPass::Render(target, cmd);

	target.ResumeScenePass(cmd);
	SceneManager::RenderWithCamera(perCamera, RenderQueueRange::Transparent, cmd);
	device_->RenderPassEnd(cmd);
}


void ou::Engine::RenderGame(const SceneRenderTarget &target, Graphics::CommandList cmd) {
	Screen::SetSize(target.GetWidth(), target.GetHeight());

	const auto *camera = SceneManager::GetMainCamera();
	auto perCamera = camera ? camera->MakePerCamera() : Camera::PerCamera{};
	FillTargetInfo(perCamera, target);
	if (camera) {
		RenderScene(target, perCamera, cmd);
	} else {
		target.BeginScenePass(cmd);
		device_->RenderPassEnd(cmd);
	}

	const VolumeStack *stack = nullptr;
	if (camera && camera->GetRenderPostProcessing()) {
		VolumeManager::Update(camera->GetTransform()->GetWorldPosition(), camera->GetVolumeLayerMask());
		stack = &VolumeManager::GetStack();
	}
	PostProcessPass::Render(target, stack, perCamera, cmd);

	target.BeginOverlayPass(cmd);
	SceneManager::RenderOverlay(cmd);
	device_->RenderPassEnd(cmd);
}


void ou::Engine::RenderSceneView(const SceneView &sceneView, Graphics::CommandList cmd) {
#ifdef WITH_EDITOR
	const auto &target = sceneView.GetRenderTarget();
	auto camera = sceneView.MakePerCamera();
	FillTargetInfo(camera, target);
	RenderScene(target, camera, cmd);

	const auto *mainCamera = SceneManager::GetMainCamera();
	VolumeManager::Update(camera.worldPosition, mainCamera ? mainCamera->GetVolumeLayerMask() : LayerMask{~0u});
	PostProcessPass::Render(target, &VolumeManager::GetStack(), camera, cmd);

	target.BeginOverlayPassWithDepth(cmd);
	GizmoRenderer::Render(camera, cmd);
	device_->RenderPassEnd(cmd);
#else
	(void)sceneView;
	(void)cmd;
#endif
}


void ou::Engine::BlitToSwapChain(const SceneRenderTarget &source, Graphics::CommandList cmd) {
	BindFullScreenViewport(cmd);
	device_->BindPipelineState(&blitPso_, cmd);

	BlitConstants constants{};
	constants.sourceIndex = static_cast<uint32_t>(device_->GetDescriptorIndex(&source.GetColor(), Graphics::ViewType::SRV));
	device_->BindDynamicConstantBuffer(constants, 0, cmd);

	device_->Draw(3, 0, cmd);
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
