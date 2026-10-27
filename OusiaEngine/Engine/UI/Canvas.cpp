#include "Canvas.h"
#include "Graphics/GraphicsDevice.h"
#include "Platform/Screen.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Shader.h"
#include "Render/Asset/Texture.h"
#include "Scene/GameObject.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"
#include "UI/CanvasRenderer.h"
#include "UI/RectTransform.h"

#include <cstring>

using namespace ou;


const Graphics::InputLayout &ou::Canvas::GetInputLayout() {
	static const Graphics::InputLayout layout = {{
		{"POSITION", 0, Graphics::Format::R32G32B32_FLOAT},
		{"COLOR", 0, Graphics::Format::R32G32B32A32_FLOAT},
		{"TEXCOORD", 0, Graphics::Format::R32G32B32A32_FLOAT},
		{"TEXCOORD", 1, Graphics::Format::R32G32B32A32_FLOAT},
	}};
	return layout;
}


void ou::Canvas::Awake() {
	rectTransform_ = dynamic_cast<RectTransform *>(GetTransform());
	if (!rectTransform_) {
		rectTransform_ = GetGameObject()->AddComponent<RectTransform>();
	}
	FitToScreen();

	SceneManager::RegisterCanvas(this);
}


void ou::Canvas::LateUpdate() {
	FitToScreen();
}


void ou::Canvas::OnDestroy() {
	SceneManager::UnregisterCanvas(this);
}


void ou::Canvas::Render(Graphics::CommandList cmd) const {
	const auto &size = rectTransform_->GetSizeDelta();
	const auto viewProj = Matrix4x4::Ortho(size.x, size.y, 0.0f, 1.0f);

	std::vector<const CanvasRenderer *> canvasRenderers;
	CollectCanvasRenderers(rectTransform_, canvasRenderers);
	for (const auto *canvasRenderer : canvasRenderers) {
		DrawCanvasRenderer(*canvasRenderer, viewProj, cmd);
	}
}


void ou::Canvas::FitToScreen() {
	const Vector2 screenSize = {
		static_cast<float>(Screen::GetWidth()),
		static_cast<float>(Screen::GetHeight())};
	if (rectTransform_->GetSizeDelta() == screenSize) { return; }

	rectTransform_->SetPivot({0.5f, 0.5f});
	rectTransform_->SetAnchoredPosition({});
	rectTransform_->SetSizeDelta(screenSize);
}


void ou::Canvas::CollectCanvasRenderers(const Transform *transform, std::vector<const CanvasRenderer *> &canvasRenderers) {
	const auto *gameObject = transform->GetGameObject();
	if (!gameObject->IsActiveInHierarchy()) { return; }

	if (const auto *canvasRenderer = gameObject->GetComponent<CanvasRenderer>()) {
		canvasRenderers.push_back(canvasRenderer);
	}

	// 子の Graphic も同じように集める
	for (const auto *child : transform->GetChildren()) {
		CollectCanvasRenderers(child, canvasRenderers);
	}
}


void ou::Canvas::DrawCanvasRenderer(const CanvasRenderer &canvasRenderer, const Matrix4x4 &viewProj, Graphics::CommandList cmd) {
	const auto &vertices = canvasRenderer.GetVertices();
	const auto &indices = canvasRenderer.GetIndices();
	if (vertices.empty() || indices.empty()) { return; }

	auto *device = Graphics::GetDevice();

	// 頂点
	const auto vertexBuffer = device->AllocateGpu(vertices.size() * sizeof(UIVertex), cmd);
	auto *gpuVertices = static_cast<UIVertex *>(vertexBuffer.data);
	for (size_t i = 0; i < vertices.size(); i++) {
		gpuVertices[i] = vertices[i];
		gpuVertices[i].color = vertices[i].color.Linear();
	}

	// インデックス
	const auto indexBuffer = device->AllocateGpu(indices.size() * sizeof(uint32_t), cmd);
	std::memcpy(indexBuffer.data, indices.data(), indices.size() * sizeof(uint32_t));

	const auto *vertexBuffers = &vertexBuffer.buffer;
	const uint32_t stride = sizeof(UIVertex);
	device->BindVertexBuffers(&vertexBuffers, 0, 1, &stride, &vertexBuffer.offset, cmd);
	device->BindIndexBuffer(&indexBuffer.buffer, Graphics::IndexBufferFormat::UINT32, indexBuffer.offset, cmd);

	const auto worldViewProj = canvasRenderer.GetTransform()->GetLocalToWorldMatrix() * viewProj;
	const auto &subMeshes = canvasRenderer.GetSubMeshes();
	for (uint32_t i = 0; i < subMeshes.size(); i++) {
		const auto &subMesh = subMeshes[i];
		const auto &material = canvasRenderer.GetMaterial(i);
		if (subMesh.indexCount == 0 || !material || !material->GetShader()) { continue; }

		const auto &texture = canvasRenderer.GetTexture() ? canvasRenderer.GetTexture()
														  : material->GetMainTexture();
		if (!texture) { continue; }

		PerCanvasRenderer constants{};
		constants.worldViewProj = worldViewProj;
		constants.color = canvasRenderer.GetColor().Linear();
		constants.textureIndex = texture->GetDescriptorIndex();
		device->BindDynamicConstantBuffer(constants, 0, cmd);

		// シェーダのプロパティ
		const auto &shader = material->GetShader();
		if (const auto size = shader->GetPropertyBufferSize(); size > 0) {
			const auto properties = device->AllocateGpu(size, cmd);
			material->WritePropertyBuffer(properties.data);
			device->BindConstantBuffer(&properties.buffer, 1, cmd, properties.offset);
		}

		for (size_t pass = 0; pass < shader->GetPassCount(); pass++) {
			const auto *pso = shader->GetPipelineState(
				pass, material->GetRenderStateOverride(), &GetInputLayout(), Graphics::PrimitiveTopology::TRIANGLELIST);
			if (!pso) { continue; }

			device->BindPipelineState(pso, cmd);
			device->DrawIndexed(subMesh.indexCount, subMesh.indexStart, 0, cmd);
		}
	}
}
