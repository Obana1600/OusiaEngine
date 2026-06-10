#include "Engine.h"
#include "TextureManager.h"
#include "input/InputManager.h"

void Engine::Initialize(const std::wstring &title, bool enableDebugLayer) {

    // COMの初期化
    CoInitializeEx(0, COINIT_MULTITHREADED);

    // ウィンドウ生成
    WinApp::GetInstance()->CreateGameWindow(title);

    // DXCommon
    DirectXCommon::GetInstance()->Initialize();

    // DXCompiler
    DXCompiler::GetInstance()->Initialize();

    // input
    InputManager::GetInstance()->Initialize();

    // imgui
    ImGuiManager::GetInstance()->Initialize();

    // texmgr
    TextureManager::GetInstance()->Initialize();
}


void Engine::Finalize() {
    // imgui
    ImGuiManager::GetInstance()->Finalize();

    // dxcommon
    DirectXCommon::GetInstance()->Finalize();

    // winapp
    WinApp::GetInstance()->TerminateGameWindow();

    // COMの終了処理
    CoUninitialize();
}


bool Engine::Update() {
    MSG msg{};

    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (msg.message == WM_QUIT) {
        return true;
    }

    InputManager::GetInstance()->Update();

    return false;
}