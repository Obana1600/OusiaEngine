#include "GameObjectCommands.h"
#include "Editor/EditorApplication.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/Selection.h"
#include "Editor/Undo.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"
#include "Serialize/SceneSerializer.h"

#include <cctype>
#include <limits>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <imgui/imgui.h>

using namespace ou;


namespace {

// 今のシーンにある、消えていない GameObject を instanceId から引く
GameObject *FindAlive(uint32_t instanceId) {
	if (instanceId == 0) { return nullptr; }
	auto *gameObject = dynamic_cast<GameObject *>(Object::FindObjectFromInstanceId(instanceId));
	if (!gameObject || gameObject->IsDestroyed() || gameObject->GetScene() != SceneManager::GetActiveScene()) { return nullptr; }
	return gameObject;
}

bool CanEdit() {
	return !EditorApplication::IsPlaying() && SceneManager::GetActiveScene();
}

// "Cube (12)" なら、base に "Cube"、number に 12 を入れる（番号がなければ false）
bool SplitNumberedName(std::string_view name, std::string_view &base, int &number) {
	if (name.size() < 4 || name.back() != ')') { return false; }

	const auto open = name.rfind(" (");
	if (open == std::string_view::npos) { return false; }

	const auto digits = name.substr(open + 2, name.size() - open - 3);
	if (digits.empty() || digits.size() > 9) { return false; }
	for (const auto c : digits) {
		if (!std::isdigit(static_cast<unsigned char>(c))) { return false; }
	}

	base = name.substr(0, open);
	number = std::stoi(std::string(digits));
	return true;
}

std::vector<GameObject *> CollectSiblings(const GameObject *self) {
	std::vector<GameObject *> siblings;
	if (auto *parent = self->GetTransform()->GetParent()) {
		for (auto *child : parent->GetChildren()) {
			siblings.push_back(child->GetGameObject());
		}
	} else if (auto *scene = self->GetScene()) {
		siblings = scene->GetRootGameObjects();
	}
	return siblings;
}

// 作ったものを選び、Undo に名前を付けて、シーンが変わったことにする
GameObject *FinishCreate(GameObject *created, std::string_view undoName) {
	if (!created) { return nullptr; }

	created->SetName(GameObjectCommands::MakeUniqueName(created->GetName(), created));
	Selection::SetActiveGameObject(created);
	Undo::SetCurrentGroupName(std::string(undoName) + " " + created->GetName());
	EditorSceneManager::MarkSceneDirty(created->GetScene());
	return created;
}

} // namespace



bool ou::GameObjectCommands::CanCopy(const GameObject *gameObject) {
	return CanEdit() && gameObject && !gameObject->IsDestroyed();
}


bool ou::GameObjectCommands::CanPaste() {
	return CanEdit() && sHasClipboard_;
}


bool ou::GameObjectCommands::CanPasteAsChild(const GameObject *parent) {
	return CanPaste() && parent && !parent->IsDestroyed();
}


bool ou::GameObjectCommands::CanDuplicate(const GameObject *gameObject) {
	return CanCopy(gameObject);
}


bool ou::GameObjectCommands::CanDelete(const GameObject *gameObject) {
	// これまでどおり、Play 中も消せる
	return gameObject && !gameObject->IsDestroyed();
}


void ou::GameObjectCommands::Copy(GameObject *gameObject) {
	if (!CanCopy(gameObject)) { return; }

	// 外への参照も書いておき、ペーストしたときに、そのときのシーンから探し直す
	sClipboard_ = SceneSerializer::ToJson(*gameObject, true);
	sHasClipboard_ = true;
	sSourceId_ = gameObject->GetInstanceId();
	const auto *parent = gameObject->GetTransform()->GetParent();
	sSourceParentId_ = parent ? parent->GetGameObject()->GetInstanceId() : 0;
}


GameObject *ou::GameObjectCommands::Paste() {
	if (!CanPaste()) { return nullptr; }

	auto *scene = SceneManager::GetActiveScene();
	auto *source = FindAlive(sSourceId_);
	auto *parent = FindAlive(sSourceParentId_);

	auto *created = SceneSerializer::InstantiateFromJson(sClipboard_, *scene, [&](GameObject &root) {
		auto *transform = root.GetTransform();
		transform->SetParent(parent ? parent->GetTransform() : nullptr, false);

		// 元が同じ親の下に残っていれば、そのすぐ下に置く
		const auto *sourceTransform = source ? source->GetTransform() : nullptr;
		if (sourceTransform && sourceTransform->GetParent() == transform->GetParent()) {
			transform->SetSiblingIndex(sourceTransform->GetSiblingIndex() + 1);
		} else {
			transform->SetAsLastSibling();
		}
	});
	return FinishCreate(created, "Paste");
}


GameObject *ou::GameObjectCommands::PasteAsChild(GameObject *parent) {
	if (!CanPasteAsChild(parent)) { return nullptr; }

	auto *created = SceneSerializer::InstantiateFromJson(sClipboard_, *parent->GetScene(), [&](GameObject &root) {
		root.GetTransform()->SetParent(parent->GetTransform(), false);
		root.GetTransform()->SetAsLastSibling();
	});
	return FinishCreate(created, "Paste");
}


GameObject *ou::GameObjectCommands::Duplicate(GameObject *gameObject) {
	if (!CanDuplicate(gameObject)) { return nullptr; }

	auto *transform = gameObject->GetTransform();
	auto *parent = transform->GetParent();
	const auto index = transform->GetSiblingIndex();

	const auto json = SceneSerializer::ToJson(*gameObject, true);
	auto *created = SceneSerializer::InstantiateFromJson(json, *gameObject->GetScene(), [&](GameObject &root) {
		root.GetTransform()->SetParent(parent, false);
		root.GetTransform()->SetSiblingIndex(index + 1);
	});
	return FinishCreate(created, "Duplicate");
}


void ou::GameObjectCommands::Delete(GameObject *gameObject) {
	if (!CanDelete(gameObject)) { return; }

	auto *scene = gameObject->GetScene();
	const auto name = gameObject->GetName();
	Object::Destroy(gameObject);

	if (Selection::GetActiveGameObject() == gameObject) {
		Selection::SetActiveGameObject(nullptr);
	}
	Undo::SetCurrentGroupName("Delete " + name);
	EditorSceneManager::MarkSceneDirty(scene);
}


void ou::GameObjectCommands::DrawMenuItems(GameObject *target) {
	if (ImGui::MenuItem("Copy", "Ctrl+C", false, CanCopy(target))) { Copy(target); }
	if (ImGui::MenuItem("Paste", "Ctrl+V", false, CanPaste())) { Paste(); }
	if (ImGui::MenuItem("Paste As Child", "Ctrl+Shift+V", false, CanPasteAsChild(target))) { PasteAsChild(target); }
	ImGui::Separator();
	if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, CanDuplicate(target))) { Duplicate(target); }
	if (ImGui::MenuItem("Delete", "Delete", false, CanDelete(target))) { Delete(target); }
}


void ou::GameObjectCommands::HandleShortcuts() {
	// Unity と同じく、Hierarchy・Scene ビューを選んでいるときだけ（Console の Ctrl+C などとぶつからないように）
	if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) { return; }

	// 文字を入力している間は、InputText が先にキーを取るので、ここには来ない
	auto *selected = Selection::GetActiveGameObject();
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C) && CanCopy(selected)) { Copy(selected); }
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_V) && CanPaste()) { Paste(); }
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_V) && CanPasteAsChild(selected)) { PasteAsChild(selected); }
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_D) && CanDuplicate(selected)) { Duplicate(selected); }
	if (ImGui::Shortcut(ImGuiKey_Delete) && CanDelete(selected)) { Delete(selected); }
}


std::string ou::GameObjectCommands::MakeUniqueName(const std::string &name, const GameObject *self) {
	std::string_view base = name;
	int number = 0;
	SplitNumberedName(name, base, number);

	// 兄弟で使っている番号（番号なしの同じ名前は 0）
	std::unordered_set<int> used;
	for (const auto *sibling : CollectSiblings(self)) {
		if (sibling == self || sibling->IsDestroyed()) { continue; }

		const std::string_view siblingName = sibling->GetName();
		std::string_view siblingBase;
		int siblingNumber = 0;
		if (siblingName == base) {
			used.insert(0);
		} else if (SplitNumberedName(siblingName, siblingBase, siblingNumber) && siblingBase == base) {
			used.insert(siblingNumber);
		}
	}

	auto next = 1;
	while (used.contains(next) && next < std::numeric_limits<int>::max()) {
		next++;
	}
	return std::string(base) + " (" + std::to_string(next) + ")";
}
