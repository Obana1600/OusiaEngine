#include "HierarchyWindow.h"
#include "Asset/AssetDatabase.h"
#include "Editor/EditorPayload.h"
#include "Editor/GameObjectMenu.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/GameObjectCommands.h"
#include "Editor/PrefabUtility.h"
#include "Editor/Selection.h"
#include "Editor/EditorWindow.h"
#include "Editor/Undo.h"
#include "Render/Asset/Model.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"

#include <algorithm>
#include <cstdint>
#include <cfloat>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

using namespace ou;


namespace {

// 押してから離すまでに、ImGui がドラッグとみなす距離を超えて動かしたか
bool WasMouseDragged() {
	const auto &io = ImGui::GetIO();
	return io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] >= io.MouseDragThreshold * io.MouseDragThreshold;
}

// target が moving 自身か、その子孫か
bool IsSelfOrDescendant(const Transform *target, const Transform *moving) {
	for (auto *p = target; p; p = p->GetParent()) {
		if (p == moving) { return true; }
	}
	return false;
}

void MarkDirty(Scene *scene, std::string_view undoName) {
	Undo::SetCurrentGroupName(undoName);
	EditorSceneManager::MarkSceneDirty(scene);
}

// 行のどこに落としたか
enum class DropPosition {
	Above,
	Inside,
	Below,
};

GameObject *FindAlive(uint32_t instanceId) {
	if (instanceId == 0) { return nullptr; }
	auto *gameObject = dynamic_cast<GameObject *>(Object::FindObjectFromInstanceId(instanceId));
	return (gameObject && !gameObject->IsDestroyed()) ? gameObject : nullptr;
}

// ancestor が target の親（親の親…）か
bool IsAncestorOf(const GameObject *ancestor, const GameObject *target) {
	for (auto *p = target->GetTransform()->GetParent(); p; p = p->GetParent()) {
		if (p->GetGameObject() == ancestor) { return true; }
	}
	return false;
}

DropPosition FindDropPosition() {
	const auto min = ImGui::GetItemRectMin();
	const auto max = ImGui::GetItemRectMax();
	const auto edge = (max.y - min.y) * 0.25f;
	const auto y = ImGui::GetMousePos().y;

	if (y < min.y + edge) { return DropPosition::Above; }
	if (y > max.y - edge) { return DropPosition::Below; }
	return DropPosition::Inside;
}

// 前後なら線、子なら枠を描く
void DrawDropIndicator(DropPosition position) {
	const auto min = ImGui::GetItemRectMin();
	const auto max = ImGui::GetItemRectMax();
	const auto color = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
	auto *drawList = ImGui::GetWindowDrawList();

	switch (position) {
		case DropPosition::Above:
			drawList->AddLine(min, {max.x, min.y}, color, 2.0f);
			break;
		case DropPosition::Inside:
			drawList->AddRect(min, max, color);
			break;
		case DropPosition::Below:
			drawList->AddLine({min.x, max.y}, max, color, 2.0f);
			break;
	}
}

GameObject *DecodeGameObject(const ImGuiPayload &payload) {
	uint32_t instanceId = 0;
	std::memcpy(&instanceId, payload.Data, sizeof(instanceId));
	return dynamic_cast<GameObject *>(Object::FindObjectFromInstanceId(instanceId));
}

} // namespace



ou::HierarchyWindow::HierarchyWindow() {
	SetTitle("Hierarchy");
}


void ou::HierarchyWindow::RequestRename(GameObject *gameObject) {
	if (!gameObject || gameObject->IsDestroyed()) { return; }
	sRenameRequestId_ = gameObject->GetInstanceId();
	EditorWindow::GetWindow<HierarchyWindow>();
}


void ou::HierarchyWindow::OnGui() {
	auto *scene = SceneManager::GetActiveScene();
	if (!scene) { return; }

	if (const auto id = std::exchange(sRenameRequestId_, 0u); id != 0) {
		if (auto *gameObject = FindAlive(id)) { BeginRename(gameObject); }
	}
	// 名前を変えている間に消されたら、やめる
	if (renamingId_ != 0 && !FindAlive(renamingId_)) { EndRename(false); }

	for (auto *gameObject : scene->GetRootGameObjects()) {
		DrawGameObject(gameObject);
	}

	DrawBackground(scene);

	// 選んでいる行を、間を空けてもう一度クリックしたら名前を変える
	if (const auto id = slowClick_.Update(); id && renamingId_ == 0) {
		auto *gameObject = FindAlive(*id);
		if (gameObject && Selection::GetActiveGameObject() == gameObject) { BeginRename(gameObject); }
	}

	GameObjectCommands::HandleShortcuts();
}


void ou::HierarchyWindow::DrawGameObject(GameObject *gameObject) {
	const auto &children = gameObject->GetTransform()->GetChildren();

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
							   ImGuiTreeNodeFlags_OpenOnDoubleClick |
							   ImGuiTreeNodeFlags_SpanAvailWidth;
	if (children.empty()) { flags |= ImGuiTreeNodeFlags_Leaf; }
	const auto isSelected = Selection::GetActiveGameObject() == gameObject;
	if (isSelected) { flags |= ImGuiTreeNodeFlags_Selected; }

	const auto isRenaming = renamingId_ == gameObject->GetInstanceId();
	// 名前の欄を行の上に重ねる
	if (isRenaming) { flags |= ImGuiTreeNodeFlags_AllowOverlap; }
	// 名前を変え始めたら、行が見えるように親を開く
	if (focusesRename_ && renamingId_ != 0) {
		if (const auto *target = FindAlive(renamingId_); target && IsAncestorOf(gameObject, target)) {
			ImGui::SetNextItemOpen(true);
		}
	}

	ImGui::PushID(static_cast<int>(gameObject->GetInstanceId()));

	const auto labelX = ImGui::GetCursorPosX() + ImGui::GetTreeNodeToLabelSpacing();
	const auto isInactive = !gameObject->IsActiveInHierarchy();
	if (isInactive) { ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)); }
	const auto isOpen = ImGui::TreeNodeEx("##Node", flags, "%s", isRenaming ? "" : gameObject->GetName().c_str());
	if (isInactive) { ImGui::PopStyleColor(); }

	if (isRenaming) {
		DrawRenameField(gameObject, labelX);
	} else {
		// Unity と同じく、選ぶのはマウスを離したとき（押した瞬間に選ぶと Inspector が切り替わり、参照の欄へ引けない）
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			isPressedOnArrow_ = ImGui::IsItemToggledOpen();
		}
		if (ImGui::IsItemDeactivated() && !isPressedOnArrow_ && !WasMouseDragged()) {
			Selection::SetActiveGameObject(gameObject);
		}
		slowClick_.TrackItem(gameObject->GetInstanceId(), isSelected);
	}

	if (!isRenaming && ImGui::BeginDragDropSource()) {
		const auto instanceId = gameObject->GetInstanceId();
		ImGui::SetDragDropPayload(EditorPayload::kGameObject, &instanceId, sizeof(instanceId));
		ImGui::TextUnformatted(gameObject->GetName().c_str());
		ImGui::EndDragDropSource();
	}
	if (ImGui::BeginDragDropTarget()) {
		AcceptDropOnRow(gameObject);
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
	// Unity と同じく、行の右クリックで作ると、その子になる
	GameObjectMenu::DrawItems(gameObject->GetScene(), gameObject);
	ImGui::Separator();
	GameObjectCommands::DrawMenuItems(gameObject);
	ImGui::EndPopup();
}


void ou::HierarchyWindow::BeginRename(GameObject *gameObject) {
	renamingId_ = gameObject->GetInstanceId();
	renamingName_ = gameObject->GetName();
	focusesRename_ = true;
	renameInactiveFrames_ = 0;
	slowClick_.Cancel();
	Focus();
}


void ou::HierarchyWindow::DrawRenameField(GameObject *gameObject, float labelX) {
	ImGui::SameLine(labelX);
	if (std::exchange(focusesRename_, false)) {
		ImGui::SetKeyboardFocusHere();
		ImGui::SetScrollHereY();
	}

	// 行の高さに収める
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {ImGui::GetStyle().FramePadding.x, 0.0f});
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputText("##Rename", &renamingName_, ImGuiInputTextFlags_AutoSelectAll);
	ImGui::PopStyleVar();

	// Enter かほかをクリックで確定、Esc で取り消し
	if (ImGui::IsItemDeactivated()) {
		EndRename(!ImGui::IsKeyPressed(ImGuiKey_Escape));
		return;
	}

	// 入力が入らないまま（ウィンドウが隠れていたなど）残らないようにする
	renameInactiveFrames_ = ImGui::IsItemActive() ? 0 : renameInactiveFrames_ + 1;
	if (renameInactiveFrames_ > 2) { EndRename(false); }
	(void)gameObject;
}


void ou::HierarchyWindow::EndRename(bool commits) {
	auto *gameObject = FindAlive(std::exchange(renamingId_, 0u));
	focusesRename_ = false;
	if (!commits || !gameObject) { return; }

	// 空にしたときは、元の名前のまま
	if (renamingName_.empty() || renamingName_ == gameObject->GetName()) { return; }

	gameObject->SetName(renamingName_);
	MarkDirty(gameObject->GetScene(), "Rename");
}


void ou::HierarchyWindow::DrawBackground(Scene *scene) {
	const auto size = ImGui::GetContentRegionAvail();
	ImGui::InvisibleButton("##Background", {size.x, std::max(size.y, ImGui::GetFrameHeight())});

	if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
		Selection::SetActiveGameObject(nullptr);
	}
	if (ImGui::BeginDragDropTarget()) {
		AcceptDropOnBackground();
		AcceptAssetDrop(scene, nullptr);
		ImGui::EndDragDropTarget();
	}
	if (ImGui::BeginPopupContextItem("##BackgroundMenu")) {
		GameObjectMenu::DrawItems(scene, nullptr);
		ImGui::Separator();
		if (ImGui::MenuItem("Paste", "Ctrl+V", false, GameObjectCommands::CanPaste())) { GameObjectCommands::Paste(); }
		ImGui::EndPopup();
	}
}


void ou::HierarchyWindow::AcceptDropOnRow(GameObject *target) {
	constexpr auto kFlags = ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect;
	const auto *payload = ImGui::AcceptDragDropPayload(EditorPayload::kGameObject, kFlags);
	if (!payload) { return; }

	auto *moving = DecodeGameObject(*payload);
	if (!moving || moving == target) { return; }

	const auto position = FindDropPosition();
	auto *movingTransform = moving->GetTransform();
	auto *targetTransform = target->GetTransform();
	auto *newParent = (position == DropPosition::Inside) ? targetTransform : targetTransform->GetParent();
	if (newParent && IsSelfOrDescendant(newParent, movingTransform)) { return; }

	DrawDropIndicator(position);
	if (!payload->IsDelivery()) { return; }

	movingTransform->SetParent(newParent, true);
	if (position != DropPosition::Inside) {
		auto index = targetTransform->GetSiblingIndex() + (position == DropPosition::Below ? 1 : 0);
		if (movingTransform->GetSiblingIndex() < index) { index--; }
		movingTransform->SetSiblingIndex(index);
	}
	MarkDirty(moving->GetScene(), "Parent Change");
}


void ou::HierarchyWindow::AcceptDropOnBackground() {
	const auto *payload = ImGui::AcceptDragDropPayload(EditorPayload::kGameObject);
	if (!payload) { return; }

	auto *moving = DecodeGameObject(*payload);
	if (!moving) { return; }

	auto *movingTransform = moving->GetTransform();
	movingTransform->SetParent(nullptr, true);
	movingTransform->SetAsLastSibling();
	MarkDirty(moving->GetScene(), "Parent Change");
}


void ou::HierarchyWindow::AcceptAssetDrop(Scene *scene, GameObject *parent) {
	const auto *payload = ImGui::AcceptDragDropPayload(EditorPayload::kAsset);
	if (!payload) { return; }

	const auto asset = EditorPayload::DecodeAsset(payload->Data, payload->DataSize);
	if (!asset.subAsset.empty()) { return; }

	GameObject *root = nullptr;
	if (const auto model = AssetDatabase::LoadAssetAtPath<Model>(asset.path)) {
		root = model->Instantiate(scene);
	} else if (const auto prefab = AssetDatabase::LoadAssetAtPath<GameObject>(asset.path)) {
		root = PrefabUtility::InstantiatePrefab(prefab.get(), scene);
	}
	if (!root) { return; }

	if (parent) {
		root->GetTransform()->SetParent(parent->GetTransform(), false);
	}

	Selection::SetActiveGameObject(root);
	MarkDirty(scene, "Instantiate " + root->GetName());
}
