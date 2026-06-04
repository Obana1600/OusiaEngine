#include "ImGuiManager.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "base/WinApp.h"
#include "base/DirectXCommon.h"


// static public func =============================================================================

ImGuiManager *ImGuiManager::GetInstance() {
	static ImGuiManager instance{};
	return &instance;
}

// ============================================================================= static public func



// public func ====================================================================================

void ImGuiManager::Initialize() {

	m_dxCommon = DirectXCommon::GetInstance();

	// SRVヒープの作成
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = m_dxCommon->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));
	assert(SUCCEEDED(hr));

	// ImGuiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(WinApp::GetInstance()->GetHwnd());
	ImGui_ImplDX12_Init(
		m_dxCommon->GetDevice(),
		static_cast<int>(m_dxCommon->GetBackBufferCount()),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		m_srvHeap.Get(),
		m_srvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_srvHeap->GetGPUDescriptorHandleForHeapStart()
	);
	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->Build();
}


void ImGuiManager::Finalize() {
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}


void ImGuiManager::Begin() {
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}


void ImGuiManager::End() {
	// imguiの内部コマンドを生成する
	ImGui::Render();
}


void ImGuiManager::Draw() {
	// 実際のcommandListのImGuiの描画コマンドリストを積む
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_dxCommon->GetCommandListForRender());
}

// ==================================================================================== public func