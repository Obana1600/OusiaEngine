#include "Engine.h"
#include "TextureManager.h"
#include "Window.h"

using namespace ou;


void Engine::Initialize(const EngineDesc &desc) {

	// COMの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	// ウィンドウ生成
	window_ = Window::Create(
		{.title = desc.title,
		 .width = desc.width,
		 .height = desc.height});

	// DXCommon
	DirectXCommon::GetInstance()->Initialize(window_.get());

	// DXCompiler
	DXCompiler::GetInstance()->Initialize();

	// input
	InputSystem::GetInstance()->Initialize(window_->Hwnd());

	// imgui
	ImGuiManager::GetInstance()->Initialize(window_->Hwnd());

	// texmgr
	TextureManager::GetInstance()->Initialize();

	// sprite
	Sprite::StaticInitialize(DirectXCommon::GetInstance()->GetDevice(), window_->ClientWidth(), window_->ClientHeight());

	// model
	Model::StaticInitialize();
}


void Engine::Finalize() {
	// imgui
	ImGuiManager::GetInstance()->Finalize();

	// model
	Model::StaticFinalize();

	// dxcommon
	DirectXCommon::GetInstance()->Finalize();

	// winapp
	window_.reset();

	// COMの終了処理
	CoUninitialize();
}


bool Engine::Update() {
	window_->PumpMessages();

	InputSystem::GetInstance()->Update();

	return window_->IsCloseRequested();
}