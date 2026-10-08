#include <Windows.h>
#include "base/Window.h"
#include "Graphics/GraphicsDevice.h"
#include <Graphics/GraphicsDevice_DX12.h>
#include "base/DXCompiler.h"


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

	struct Vertex {
		float x, y, z;
	};
	const Vertex vertices[] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};

	GpuBufferDesc bufferDesc;
	bufferDesc.size = sizeof(vertices);
	bufferDesc.heapType = HeapType::DEFAULT;
	bufferDesc.bindFlags = BindFlag::VERTEX_BUFFER;

	GpuBuffer testBuffer;
	device.CreateBuffer(&bufferDesc, vertices, &testBuffer);
	device.SetName(&testBuffer, "TestVertexBuffer");

	DXCompiler::GetInstance()->Initialize();

	Shader vs;
	Shader ps;
	{
		auto vsBlob = DXCompiler::GetInstance()->CompileShader(L"Triangle.VS.hlsl", L"vs_6_6");
		auto psBlob = DXCompiler::GetInstance()->CompileShader(L"Triangle.PS.hlsl", L"ps_6_6");
		device.CreateShader(ShaderStage::VS, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &vs);
		device.CreateShader(ShaderStage::PS, psBlob->GetBufferPointer(), psBlob->GetBufferSize(), &ps);
	} // ここで Blob は捨ててよい

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