#include <Windows.h>
#include "base/Window.h"
#include "Graphics/GraphicsDevice.h"
#include <Graphics/GraphicsDevice_DX12.h>


// Windowsアプリでのエントリーポイント（main関数）
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	using namespace ou;

	auto window = Window::Create({.title = "GraphicsDevice テスト"});

	GraphicsDevice_DX12 device(ValidationMode::Enabled);
	GetDevice() = &device;

	SwapChainDesc swapChainDesc;
	swapChainDesc.width = window->ClientWidth();
	swapChainDesc.height = window->ClientHeight();
	swapChainDesc.bufferCount = 2;
	swapChainDesc.format = Format::R8G8B8A8_UNORM_SRGB;
	swapChainDesc.clearColor[0] = 0.1f;
	swapChainDesc.clearColor[1] = 0.25f;
	swapChainDesc.clearColor[2] = 0.5f;
	swapChainDesc.clearColor[3] = 1.0f;

	SwapChain swapChain;
	device.CreateSwapChain(&swapChainDesc, window->Hwnd(), &swapChain);

	while (!window->IsCloseRequested()) {
		window->PumpMessages();

		// 最小化中は描かない
		if (window->IsMinimized()) {
			Sleep(10);
			continue;
		}

		// ウィンドウの大きさが変わったら、CreateSwapChain をもう一度呼んでリサイズする
		if (window->ClientWidth() != swapChain.GetDesc().width ||
			window->ClientHeight() != swapChain.GetDesc().height) {
			SwapChainDesc desc = swapChain.GetDesc();
			desc.width = window->ClientWidth();
			desc.height = window->ClientHeight();
			device.CreateSwapChain(&desc, window->Hwnd(), &swapChain);
		}

		CommandList cmd = device.BeginCommandList();
		device.RenderPassBegin(&swapChain, cmd);
		device.RenderPassEnd(cmd);
		device.SubmitCommandLists();
	}

	GetDevice() = nullptr;
	return 0;
}