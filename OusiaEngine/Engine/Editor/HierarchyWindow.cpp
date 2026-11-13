#include "HierarchyWindow.h"
#include "Asset/AssetDatabase.h"
#include "Editor/EditorPayload.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/Selection.h"
#include "Render/Asset/Model.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

#include <imgui/imgui.h>

using namespace ou;


namespace {

// target が moving 自身か、その子孫か
bool IsSelfOrDescendant(const Transform *target, const Transform *moving) {
	for (auto *p = target; p; p = p->GetParent()) {
		if (p == moving) { return true; }
	}
	return false;
}

void MarkDirty(Scene *scene) {
	EditorSceneManager::MarkSceneDirty(scene);
}

} // namespace



ou::HierarchyWindow::HierarchyWindow() {
	SetTitle("Hierarchy");
}


void ou::HierarchyWindow::OnGui() {
	auto *scene = SceneManager::GetActiveScene();
	if (!scene) { return; }

	for (auto *gameObject : scene->GetRootGameObjects()) {
		DrawGameObject(gameObject);
	}

	DrawBackground(scene);

	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
		if (auto *selected = Selection::GetActiveGameObject()) { Delete(selected); }
	}
}


void ou::HierarchyWindow::DrawGameObject(GameObject *gameObject) {
	const auto &children = gameObject->GetTransform()->GetChildren();

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
							   ImGuiTreeNodeFlags_OpenOnDoubleClick |
							   ImGuiTreeNodeFlags_SpanAvailWidth;
	if (children.empty()) { flags |= ImGuiTreeNodeFlags_Leaf; }
	if (Selection::GetActiveGameObject() == gameObject) { flags |= ImGuiTreeNodeFlags_Selected; }

	ImGui::PushID(static_cast<int>(gameObject->GetInstanceId()));

	const auto isInactive = !gameObject->IsActiveInHierarchy();
	if (isInactive) { ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)); }
	const auto isOpen = ImGui::TreeNodeEx("##Node", flags, "%s", gameObject->GetName().c_str());
	if (isInactive) { ImGui::PopStyleColor(); }

	if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) {
		Selection::SetActiveGameObject(gameObject);
	}

	if (ImGui::BeginDragDropSource()) {
		const auto instanceId = gameObject->GetInstanceId();
		ImGui::SetDragDropPayload(EditorPayload::kGameObject, &instanceId, sizeof(instanceId));
		ImGui::TextUnformatted(gameObject->GetName().c_str());
		ImGui::EndDragDropSource();
	}
	if (ImGui::BeginDragDropTarget()) {
		AcceptDrop(gameObject);
		AcceptAssetDrop(gameObject->GetScene(), gameObject);
		ImGui::EndDragDropTarget();
	}

	DrawContextMenu(gameObject);

	if (isOpen) {
		const auto childrenCopy = children;
		for (auto *child : childrenCopy) {
			DrawGameObject(child->GetGameObject());
		}
		ImGui::TreePop();
	}

	ImGui::PopID();
}


void ou::HierarchyWindow::DrawContextMenu(GameObject *gameObject) {
	if (!ImGui::BeginPopupContextItem()) { return; }

	Selection::SetActiveGameObject(gameObject);
	if (ImGui::MenuItem("Create Empty Child")) { CreateEmpty(gameObject->GetScene(), gameObject); }
	if (ImGui::MenuItem("Delete", "Delete")) { Delete(gameObject); }
	ImGui::EndPopup();
}


void ou::HierarchyWindow::DrawBackground(Scene *scene) {
	const auto size = ImGui::GetContentRegionAvail();
	ImGui::InvisibleButton("##Background", {size.x, std::max(size.y, ImGui::GetFrameHeight())});

	if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
		Selection::SetActiveGameObject(nullptr);
	}
	if (ImGui::BeginDragDropTarget()) {
		AcceptDrop(nullptr);
		AcceptAssetDrop(scene, nullptr);
		ImGui::EndDragDropTarget();
	}
	if (ImGui::BeginPopupContextItem("##BackgroundMenu")) {
		if (ImGui::MenuItem("Create Empty")) { CreateEmpty(scene, nullptr); }
		ImGui::EndPopup();
	}
}


void ou::HierarchyWindow::CreateEmpty(Scene *scene, GameObject *parent) {
	auto *gameObject = scene->CreateGameObject("GameObject");
	if (parent) {
		gameObject->GetTransform()->SetParent(parent->GetTransform(), false);
	}

	Selection::SetActiveGameObject(gameObject);
	MarkDirty(scene);
}


void ou::HierarchyWindow::Delete(GameObject *gameObject) {
	auto *scene = gameObject->GetScene();
	Object::Destroy(gameObject);

	if (Selection::GetActiveGameObject() == gameObject) {
		Selection::SetActiveGameObject(nullptr);
	}
	MarkDirty(scene);
}


void ou::HierarchyWindow::AcceptDrop(GameObject *parent) {
	const auto *payload = ImGui::AcceptDragDropPayload(EditorPayload::kGameObject);
	if (!payload) { return; }

	uint32_t instanceId = 0;
	std::memcpy(&instanceId, payload->Data, sizeof(instanceId));
	auto *moving = dynamic_cast<GameObject *>(Object::FindObjectFromInstanceId(instanceId));
	if (!moving) { return; }

	auto *movingTransform = moving->GetTransform();
	auto *parentTransform = parent ? parent->GetTransform() : nullptr;
	if (parentTransform && IsSelfOrDescendant(parentTransform, movingTransform)) { return; }

	movingTransform->SetParent(parentTransform, true);
	MarkDirty(moving->GetScene());
}


void ou::HierarchyWindow::AcceptAssetDrop(Scene *scene, GameObject *parent) {
	const auto *payload = ImGui::AcceptDragDropPayload(EditorPayload::kAsset);
	if (!payload) { return; }

	const auto asset = EditorPayload::DecodeAsset(payload->Data, payload->DataSize);
	if (!asset.subAsset.empty()) { return; }
	const auto model = AssetDatabase::LoadAssetAtPath<Model>(asset.path);
	if (!model) { return; }

	auto *root = model->Instantiate(scene);
	if (parent) {
		root->GetTransform()->SetParent(parent->GetTransform(), false);
	}

	Selection::SetActiveGameObject(root);
	MarkDirty(scene);
}
