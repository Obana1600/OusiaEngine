#include "Core/Fatal.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/GraphicsDevice_DX12.h"
#include "base/DXCompiler.h"
#include "base/Window.h"
#include <Windows.h>


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

	DXCompiler::GetInstance()->Initialize();

	Shader vs;
	Shader ps;
	{
		auto vsBlob = DXCompiler::GetInstance()->CompileShader(L"Triangle.VS.hlsl", L"vs_6_6");
		auto psBlob = DXCompiler::GetInstance()->CompileShader(L"Triangle.PS.hlsl", L"ps_6_6");
		device.CreateShader(ShaderStage::VS, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &vs);
		device.CreateShader(ShaderStage::PS, psBlob->GetBufferPointer(), psBlob->GetBufferSize(), &ps);
	} // ここで Blob は捨ててよい

	InputLayout inputLayout;
	inputLayout.elements = {
		{.semanticName = "POSITION", .format = Format::R32G32B32_FLOAT},
		{.semanticName = "COLOR", .format = Format::R32G32B32A32_FLOAT},
	};

	PipelineStateDesc psoDesc;
	psoDesc.vs = &vs;
	psoDesc.ps = &ps;
	psoDesc.il = &inputLayout;
	psoDesc.pt = PrimitiveTopology::TRIANGLELIST;

	PipelineState pso;
	const RenderPassInfo renderPassInfo = RenderPassInfo::From(swapChain.GetDesc());
	if (!device.CreatePipelineState(&psoDesc, &pso, &renderPassInfo)) {
		FatalError("PSO の作成に失敗しました。");
	}

	struct Vertex {
		float position[3];
		float color[4];
	};
	Vertex vertices[] = {
		{{0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
	};

	GpuBufferDesc vbDesc;
	vbDesc.size = sizeof(vertices);
	vbDesc.heapType = HeapType::DEFAULT;
	vbDesc.bindFlags = BindFlag::VERTEX_BUFFER;

	GpuBuffer vertexBuffer;
	device.CreateBuffer(&vbDesc, vertices, &vertexBuffer);
	device.SetName(&vertexBuffer, "TriangleVB");

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
		{
			const SwapChainDesc &scDesc = swapChain.GetDesc();

			Viewport viewport;
			viewport.width = static_cast<float>(scDesc.width);
			viewport.height = static_cast<float>(scDesc.height);
			device.BindViewports(1, &viewport, cmd);

			Scissor scissor;
			scissor.right = static_cast<int32_t>(scDesc.width);
			scissor.bottom = static_cast<int32_t>(scDesc.height);
			device.BindScissorRects(1, &scissor, cmd);

			device.BindPipelineState(&pso, cmd);

			const GpuBuffer *vbs[] = {&vertexBuffer};
			const uint32_t strides[] = {sizeof(Vertex)};
			device.BindVertexBuffers(vbs, 0, 1, strides, nullptr, cmd);

			device.Draw(3, 0, cmd);
		}
		device.RenderPassEnd(cmd);
		device.SubmitCommandLists();
	}

	GetDevice() = nullptr;
	return 0;
}