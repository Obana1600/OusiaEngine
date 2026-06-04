#include "Engine.h"

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

    // PSO
    PipelineState pipelineState{};
    RootSignature rootSignature = RootSignature(DirectXCommon::GetInstance()->GetDevice());

    pipelineState.SetRootSignature(rootSignature.Get());
    pipelineState.SetInputLayout(VertexData::InputLayout);
    pipelineState.SetVS(L"Object3d.VS.hlsl", L"vs_6_0");
    pipelineState.SetPS(L"Object3d.PS.hlsl", L"ps_6_0");
    pipelineState.Create(DirectXCommon::GetInstance()->GetDevice());
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
        return true;  // ループ脱出
    }

    return false;  // ループ継続
}