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
#include "Script/MonoScript.h"
#include "Script/MonoBehavior.h"
#include "Serialize/TypeRegistry.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cfloat>
#include <cstring>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
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

// 大文字・小文字を区別せずに text を探す（なければ npos）
size_t FindIgnoreCase(std::string_view source, std::string_view text) {
	if (text.empty()) { return 0; }
	const auto it = std::search(source.begin(), source.end(), text.begin(), text.end(), [](char a, char b) {
		return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
	});
	return it == source.end() ? std::string_view::npos : static_cast<size_t>(it - source.begin());
}

// コンポーネントの型の名前に type が含まれるか（スクリプトは、スクリプトの名前でも当てる）
bool HasComponentOfType(const GameObject &gameObject, std::string_view type) {
	for (auto *component : gameObject.GetComponents<Component>()) {
		if (component->IsDestroyed()) { continue; }

		if (const auto *name = TypeRegistry::FindName(*component); name && FindIgnoreCase(*name, type) != std::string_view::npos) {
			return true;
		}
		if (auto *behavior = dynamic_cast<MonoBehavior *>(component)) {
			const auto *script = MonoScript::FromMonoBehavior(behavior);
			if (script && FindIgnoreCase(script->GetName(), type) != std::string_view::npos) { return true; }
		}
	}
	return false;
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

	DrawSearchField();
	UpdateSearchResults(scene);
	// 検索で隠れているものの名前を変えるときは、検索を消して出す
	if (renamingId_ != 0 && IsSearching() && !visibleIds_.contains(renamingId_)) {
		ClearSearch();
		UpdateSearchResults(scene);
	}

	// 検索欄はスクロールしないように、ツリーは子ウィンドウに描く
	ImGui::BeginChild("##Tree");

	// 検索中は開いている状態を別に持つ（検索を消したら、元の開き方に戻る）
	if (IsSearching()) { ImGui::PushID("##Search"); }
	for (auto *gameObject : scene->GetRootGameObjects()) {
		DrawGameObject(gameObject);
	}
	if (IsSearching()) { ImGui::PopID(); }
	opensSearchResults_ = false;
	revealId_ = 0;

	DrawBackground(scene);

	// 選んでいる行を、間を空けてもう一度クリックしたら名前を変える
	if (const auto id = slowClick_.Update(); id && renamingId_ == 0) {
		auto *gameObject = FindAlive(*id);
		if (gameObject && Selection::GetActiveGameObject() == gameObject) { BeginRename(gameObject); }
	}

	GameObjectCommands::HandleShortcuts();

	ImGui::EndChild();
}


void ou::HierarchyWindow::DrawSearchField() {
	// Unity と同じく、Hierarchy を選んでいるときの Ctrl+F で検索欄に入る
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_F)) {
		focusesSearch_ = true;
	}
	if (std::exchange(focusesSearch_, false)) { ImGui::SetKeyboardFocusHere(); }

	const auto hasText = !searchText_.empty();
	const auto clearWidth = ImGui::GetFrameHeight();
	ImGui::SetNextItemWidth(hasText ? -(clearWidth + ImGui::GetStyle().ItemSpacing.x) : -FLT_MIN);
	// Esc で消す（もう一度 Esc で欄から出る）
	ImGui::InputTextWithHint("##Search", "Search (t:Type)", &searchText_, ImGuiInputTextFlags_EscapeClearsAll);

	if (hasText) {
		ImGui::SameLine();
		if (ImGui::Button("X##ClearSearch", {clearWidth, 0.0f})) { ClearSearch(); }
		if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Clear search"); }
	}
}


void ou::HierarchyWindow::UpdateSearchResults(Scene *scene) {
	if (searchText_ != previousSearchText_) {
		// 検索を消したら、選んでいるものが見えるところまで開く
		if (searchText_.empty()) {
			if (auto *selected = Selection::GetActiveGameObject(); selected && !selected->IsDestroyed()) {
				revealId_ = selected->GetInstanceId();
			}
		}
		opensSearchResults_ = true;
		previousSearchText_ = searchText_;

		// "t:" で始まる語は型、ほかは空白 1 つでつないで名前として探す（"Main Camera" も探せるように）
		searchQuery_ = {};
		std::istringstream stream(searchText_);
		std::string word;
		while (stream >> word) {
			if (word.size() >= 2 && (word[0] == 't' || word[0] == 'T') && word[1] == ':') {
				if (word.size() > 2) { searchQuery_.types.push_back(word.substr(2)); }
				continue;
			}
			if (!searchQuery_.name.empty()) { searchQuery_.name += ' '; }
			searchQuery_.name += word;
		}
	}

	matchedIds_.clear();
	visibleIds_.clear();
	if (!IsSearching()) { return; }

	// 子から先に見て、当てはまるものか、当てはまる子孫を持つものを出す
	const auto visit = [&](auto &self, GameObject *gameObject) -> bool {
		if (gameObject->IsDestroyed()) { return false; }

		auto hasVisibleChild = false;
		for (auto *child : gameObject->GetTransform()->GetChildren()) {
			hasVisibleChild |= self(self, child->GetGameObject());
		}

		const auto matchesName = FindIgnoreCase(gameObject->GetName(), searchQuery_.name) != std::string_view::npos;
		const auto matchesTypes = std::all_of(searchQuery_.types.begin(), searchQuery_.types.end(), [&](const std::string &type) {
			return HasComponentOfType(*gameObject, type);
		});
		const auto id = gameObject->GetInstanceId();
		if (matchesName && matchesTypes) { matchedIds_.insert(id); }

		const auto isVisible = hasVisibleChild || matchedIds_.contains(id);
		if (isVisible) { visibleIds_.insert(id); }
		return isVisible;
	};
	for (auto *root : scene->GetRootGameObjects()) {
		visit(visit, root);
	}
}


void ou::HierarchyWindow::ClearSearch() {
	searchText_.clear();
	// 入力中なら欄から出す（入力中の欄は自分で持っている文字を使うので、消したことが伝わらない）
	if (ImGui::GetActiveID() == ImGui::GetID("##Search")) { ImGui::ClearActiveID(); }
}


void ou::HierarchyWindow::DrawSearchHighlight(const GameObject *gameObject, float labelScreenX) {
	if (searchQuery_.name.empty() || !matchedIds_.contains(gameObject->GetInstanceId())) { return; }

	const auto &name = gameObject->GetName();
	const auto position = FindIgnoreCase(name, searchQuery_.name);
	if (position == std::string_view::npos) { return; }

	const auto *begin = name.c_str();
	const auto x0 = labelScreenX + ImGui::CalcTextSize(begin, begin + position).x;
	const auto x1 = x0 + ImGui::CalcTextSize(begin + position, begin + position + searchQuery_.name.size()).x;
	const auto min = ImGui::GetItemRectMin();
	const auto max = ImGui::GetItemRectMax();
	ImGui::GetWindowDrawList()->AddRectFilled({x0, min.y + 1.0f}, {x1, max.y - 1.0f}, ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
}


void ou::HierarchyWindow::DrawGameObject(GameObject *gameObject) {
	const auto id = gameObject->GetInstanceId();
	const auto isSearching = IsSearching();
	if (isSearching && !visibleIds_.contains(id)) { return; }

	const auto &children = gameObject->GetTransform()->GetChildren();
	// 検索中は、出す子があるときだけ開けるようにする
	const auto hasVisibleChild = isSearching
		? std::any_of(children.begin(), children.end(), [&](auto *child) { return visibleIds_.contains(child->GetGameObject()->GetInstanceId()); })
		: !children.empty();

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
							   ImGuiTreeNodeFlags_OpenOnDoubleClick |
							   ImGuiTreeNodeFlags_SpanAvailWidth;
	if (!hasVisibleChild) { flags |= ImGuiTreeNodeFlags_Leaf; }
	// 検索の文字が変わったら、当てはまるものが見えるように開く（そのあとは閉じてもよい）
	if (isSearching && hasVisibleChild && opensSearchResults_) { ImGui::SetNextItemOpen(true, ImGuiCond_Always); }
	// 検索を消したら、選んでいるものが見えるように親を開く
	if (revealId_ != 0) {
		if (const auto *target = FindAlive(revealId_); target && IsAncestorOf(gameObject, target)) {
			ImGui::SetNextItemOpen(true);
		}
	}
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
	const auto labelScreenX = ImGui::GetCursorScreenPos().x + ImGui::GetTreeNodeToLabelSpacing();
	// 非アクティブなものと、検索で当てはまらない親（場所を見せるためだけに出す）は薄く出す
	const auto isDimmed = !gameObject->IsActiveInHierarchy() || (isSearching && !matchedIds_.contains(id));
	if (isDimmed) { ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)); }
	const auto isOpen = ImGui::TreeNodeEx("##Node", flags, "%s", isRenaming ? "" : gameObject->GetName().c_str());
	if (isDimmed) { ImGui::PopStyleColor(); }
	if (isSearching && !isRenaming) { DrawSearchHighlight(gameObject, labelScreenX); }
	if (revealId_ == id) { ImGui::SetScrollHereY(); }

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
