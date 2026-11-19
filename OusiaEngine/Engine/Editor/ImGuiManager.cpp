#include "ImGuiManager.h"

#ifdef USE_IMGUI
#include "Core/StringUtil.h"
#include "Graphics/DirectX12/GraphicsDevice_DX12.h"
#include "Platform/Paths.h"

// clang-format off
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx12.h>
#include <imgui/imgui_impl_win32.h>
#include <ImGuizmo/ImGuizmo.h> // imgui/imgui.h の後ろに置く
// clang-format on

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
#endif

using namespace ou;


#ifdef USE_IMGUI
namespace {

Graphics::GraphicsDevice_DX12 *GetDeviceDx12() {
	return static_cast<Graphics::GraphicsDevice_DX12 *>(Graphics::GetDevice());
}

void AllocateSrv(ImGui_ImplDX12_InitInfo *info,
				 D3D12_CPU_DESCRIPTOR_HANDLE *outCpu,
				 D3D12_GPU_DESCRIPTOR_HANDLE *outGpu) {
	auto *device = static_cast<Graphics::GraphicsDevice_DX12 *>(info->UserData);
	const auto index = device->AllocateBindlessIndex();
	*outCpu = device->GetBindlessCpuHandle(index);
	*outGpu = device->GetBindlessGpuHandle(index);
}

void FreeSrv(ImGui_ImplDX12_InitInfo *info,
			 D3D12_CPU_DESCRIPTOR_HANDLE,
			 D3D12_GPU_DESCRIPTOR_HANDLE gpu) {
	auto *device = static_cast<Graphics::GraphicsDevice_DX12 *>(info->UserData);
	const auto start = device->GetBindlessGpuHandle(0).ptr;
	const auto stride = device->GetBindlessGpuHandle(1).ptr - start;
	device->FreeBindlessIndex(static_cast<int>((gpu.ptr - start) / stride));
}

} // namespace
#endif



ou::ImGuiManager::ImGuiManager(Window *window)
	: window_(window) {
#ifdef USE_IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	// 折りたたみ（ドッキング時はウィンドウメニュー）のボタンを出さない
	ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;

	// ドッキングの有効化 / 日本語フォントの追加
	auto &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// レイアウトは PC ごとの設定なので、カレントディレクトリではなく UserSettings に置く（Unity の UserSettings/Layouts と同じ）
	if (const auto userSettings = FindUserSettingsFolder()) {
		iniPath_ = ToUtf8(*userSettings / "imgui.ini");
		io.IniFilename = iniPath_.c_str();
	}
	if (const auto engineAssets = FindEngineAssetsFolder()) {
		const auto fontPath = ToUtf8(*engineAssets / "Font" / kUseJpFontName);
		io.Fonts->AddFontFromFileTTF(fontPath.c_str(), kFontSize);
	}

	// win32
	ImGui_ImplWin32_Init(window_->Hwnd());
	messageHandlerId_ = window_->AddMessageHandler(
		[](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> std::optional<LRESULT> {
			if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
				return TRUE;
			}
			return std::nullopt;
		});

	// dx12
	auto *device = GetDeviceDx12();
	ImGui_ImplDX12_InitInfo info{};
	info.Device = device->GetNativeDevice();
	info.CommandQueue = device->GetNativeCommandQueue(Graphics::QUEUE_GRAPHICS);
	info.NumFramesInFlight = Graphics::GraphicsDevice::BUFFER_COUNT;
	info.RTVFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	info.DSVFormat = DXGI_FORMAT_UNKNOWN;
	info.UserData = device;
	info.SrvDescriptorHeap = device->GetBindlessHeap();
	info.SrvDescriptorAllocFn = &AllocateSrv;
	info.SrvDescriptorFreeFn = &FreeSrv;
	ImGui_ImplDX12_Init(&info);
#endif
}


ou::ImGuiManager::~ImGuiManager() {
#ifdef USE_IMGUI
	ImGui_ImplDX12_Shutdown();
	window_->RemoveMessageHandler(messageHandlerId_);
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif
}


void ou::ImGuiManager::NewFrame() {
#ifdef USE_IMGUI
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
#endif
}


void ou::ImGuiManager::EndFrame() {
#ifdef USE_IMGUI
	ImGui::EndFrame();
#endif
}


void ou::ImGuiManager::Render(Graphics::CommandList cmd) {
#ifdef USE_IMGUI
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GetDeviceDx12()->GetNativeCommandList(cmd));
#else
	(void)cmd;
#endif
}
