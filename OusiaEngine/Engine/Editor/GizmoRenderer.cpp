#include "GizmoRenderer.h"
#include "Asset/AssetDatabase.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorFunc.h"
#include "Editor/Selection.h"
#include "Math/Mathf.h"
#include "Render/Asset/Shader.h"
#include "Render/Asset/Texture.h"
#include "Render/Gizmos.h"
#include "Scene/Behavior.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <vector>

#include <imgui/imgui.h>

using namespace ou;


namespace {

// アイコンの、画面での大きさ（ピクセル）
constexpr float kIconSize = 32.0f;
// allowScaling のアイコンは、この距離より遠いと小さくする（半分まで）
constexpr float kIconScaleDistance = 10.0f;
// 物の裏に隠れた線の濃さ
constexpr float kHiddenAlpha = 0.25f;

} // namespace



void ou::GizmoRenderer::SetEnabled(bool isEnabled) {
	sIsEnabled_ = isEnabled;
}


bool ou::GizmoRenderer::IsEnabled() {
	return sIsEnabled_;
}


void ou::GizmoRenderer::Collect(Scene *scene) {
	Gizmos::Clear();
	if (!sIsEnabled_ || !scene) { return; }

	const auto *selected = Selection::GetActiveGameObject();
	for (auto *root : scene->GetRootGameObjects()) {
		DrawGameObject(root, selected, false);
	}
}


void ou::GizmoRenderer::DrawIcons(const Camera::PerCamera &camera, const Rect &sceneRect, ImDrawList *drawList) {
	if (!sIsEnabled_ || Gizmos::sIcons_.empty()) { return; }

	struct Item {
		float depth;
		ImVec2 min;
		ImVec2 max;
		Texture *texture;
		ImU32 color;
	};
	std::vector<Item> items;

	const auto &m = camera.viewProjection;
	for (const auto &icon : Gizmos::sIcons_) {
		auto *texture = FindIconTexture(icon.name);
		if (!texture) { continue; }

		// 行ベクトルに右から掛ける（シェーダーの mul(float4(p, 1), gViewProjection) と同じ）
		const auto &p = icon.position;
		const auto x = p.x * m.m11 + p.y * m.m21 + p.z * m.m31 + m.m41;
		const auto y = p.x * m.m12 + p.y * m.m22 + p.z * m.m32 + m.m42;
		const auto w = p.x * m.m14 + p.y * m.m24 + p.z * m.m34 + m.m44;
		// カメラの後ろは出さない
		if (w <= 1e-4f) { continue; }

		const auto screenX = sceneRect.x + (x / w * 0.5f + 0.5f) * sceneRect.width;
		const auto screenY = sceneRect.y + (0.5f - y / w * 0.5f) * sceneRect.height;
		const auto size = icon.allowScaling ? kIconSize * Mathf::Clamp(kIconScaleDistance / w, 0.5f, 1.0f) : kIconSize;
		const auto half = size * 0.5f;

		const ImVec4 tint{icon.tint.r, icon.tint.g, icon.tint.b, icon.tint.a};
		items.push_back(Item{w, {screenX - half, screenY - half}, {screenX + half, screenY + half}, texture, ImGui::GetColorU32(tint)});
	}

	// 遠いものから描き、近いものを上に重ねる
	std::sort(items.begin(), items.end(), [](const Item &a, const Item &b) { return a.depth > b.depth; });

	drawList->PushClipRect({sceneRect.x, sceneRect.y}, {sceneRect.x + sceneRect.width, sceneRect.y + sceneRect.height}, true);
	for (const auto &item : items) {
		drawList->AddImage(ToTextureId(item.texture->GetGraphicsTexture()), item.min, item.max, {0.0f, 0.0f}, {1.0f, 1.0f}, item.color);
	}
	drawList->PopClipRect();
}


void ou::GizmoRenderer::StaticInitialize() {
	sShader_ = AssetDatabase::LoadAssetAtPath<Shader>("EngineAssets/Shader/Gizmo.shader");
	if (!sShader_) {
		Log::Error("Gizmo のシェーダの読み込みに失敗しました。Scene ビューに Gizmo の線が出ません");
	}
}


void ou::GizmoRenderer::StaticFinalize() {
	sShader_ = nullptr;
	for (auto &buffer : sVertexBuffers_) {
		buffer = {};
	}
	sCapacity_ = 0;
	sIconTextures_.clear();
	Gizmos::Clear();
}


void ou::GizmoRenderer::Render(const Camera::PerCamera &camera, Graphics::CommandList cmd) {
	const auto &vertices = Gizmos::sVertices_;
	if (!sIsEnabled_ || !sShader_ || vertices.empty()) { return; }

	const auto count = static_cast<uint32_t>(vertices.size());
	EnsureCapacity(count);

	auto *device = Graphics::GetDevice();
	auto &buffer = sVertexBuffers_[device->GetBufferIndex()];
	std::memcpy(buffer.mappedData, vertices.data(), sizeof(Gizmos::Vertex) * count);

	device->BindDynamicConstantBuffer(camera, 2, cmd);

	PerGizmo constants{};
	constants.vertexBufferIndex = static_cast<uint32_t>(device->GetDescriptorIndex(&buffer, Graphics::ViewType::SRV));

	const auto &passes = sShader_->GetPasses();
	for (size_t pass = 0; pass < passes.size(); pass++) {
		const auto *pso = sShader_->GetPipelineState(pass, {}, nullptr, Graphics::PrimitiveTopology::LINELIST);
		if (!pso) { continue; }

		constants.alphaScale = passes[pass].name == "Hidden" ? kHiddenAlpha : 1.0f;
		device->BindDynamicConstantBuffer(constants, 0, cmd);
		device->BindPipelineState(pso, cmd);
		device->Draw(count, 0, cmd);
	}
}


void ou::GizmoRenderer::EnsureCapacity(uint32_t count) {
	if (count <= sCapacity_) { return; }

	sCapacity_ = std::max({count, sCapacity_ * 2, 1024u});

	Graphics::GpuBufferDesc desc{};
	desc.size = sCapacity_ * sizeof(Gizmos::Vertex);
	desc.heapType = Graphics::HeapType::UPLOAD;
	desc.bindFlags = Graphics::BindFlag::SHADER_RESOURCE;
	desc.miscFlags = Graphics::ResourceMiscFlag::BUFFER_STRUCTURED;
	desc.stride = sizeof(Gizmos::Vertex);

	for (auto &buffer : sVertexBuffers_) {
		if (!Graphics::GetDevice()->CreateBuffer(&desc, nullptr, &buffer)) {
			Log::Error("Gizmo の線のバッファの作成に失敗しました");
			sCapacity_ = 0;
			return;
		}
	}
}


Texture *ou::GizmoRenderer::FindIconTexture(const std::string &name) {
	if (const auto it = sIconTextures_.find(name); it != sIconTextures_.end()) { return it->second.get(); }

	// Unity と同じく、プロジェクトの Assets/Gizmos/ を先に探す（同じ名前で、組み込みのアイコンを差し替えられる）
	auto fileName = name;
	if (std::filesystem::path(fileName).extension().empty()) { fileName += ".png"; }

	std::shared_ptr<Texture> texture;
	for (const char *folder : {"Assets/Gizmos/", "EngineAssets/Gizmos/"}) {
		const auto path = folder + fileName;
		if (!AssetDatabase::AssetPathToGuid(path).IsValid()) { continue; }
		if ((texture = AssetDatabase::LoadAssetAtPath<Texture>(path))) { break; }
	}
	if (!texture) {
		Log::Warning("Gizmo のアイコンが見つかりません: {} (Assets/Gizmos/ に置いてください)", name);
	}

	return sIconTextures_.emplace(name, std::move(texture)).first->second.get();
}


// 選んでいるもの、またはその親を選んでいるときに OnDrawGizmosSelected を呼ぶ（Unity と同じ）
void ou::GizmoRenderer::DrawGameObject(GameObject *gameObject, const GameObject *selected, bool isParentSelected) {
	if (gameObject->IsDestroyed() || !gameObject->IsActiveInHierarchy()) { return; }

	const auto isSelected = isParentSelected || gameObject == selected;
	for (auto *component : gameObject->GetComponents<Component>()) {
		if (component->IsDestroyed()) { continue; }
		// Unity と同じく、切っているスクリプトなどは描かない
		if (const auto *behavior = dynamic_cast<const Behavior *>(component); behavior && !behavior->IsEnabled()) { continue; }

		Gizmos::BeginDraw(gameObject);
		component->OnDrawGizmos();
		if (isSelected) { component->OnDrawGizmosSelected(); }
		Gizmos::EndDraw();
	}

	// スクリプトが OnDrawGizmos で子を変えても大丈夫なように、写してから回る
	std::vector<GameObject *> children;
	for (auto *child : gameObject->GetTransform()->GetChildren()) {
		children.push_back(child->GetGameObject());
	}
	for (auto *child : children) {
		DrawGameObject(child, selected, isSelected);
	}
}
