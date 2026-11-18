#include "InspectorWindow.h"
#include "Asset/AssetDatabase.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/Selection.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"
#include "Scene/LayerMask.h"
#include "Scene/Transform.h"
#include "Script/MonoBehavior.h"
#include "Script/MonoScript.h"
#include "Serialize/TypeRegistry.h"
#include "Core/StringUtil.h"
#include "Editor/EditorGUILayout.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <typeinfo>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

using namespace ou;


namespace {

void MarkDirty(GameObject *gameObject) {
	EditorSceneManager::MarkSceneDirty(gameObject->GetScene());
}

bool MatchesFilter(std::string_view name, std::string_view filter) {
	const auto it = std::search(name.begin(), name.end(), filter.begin(), filter.end(), [](char a, char b) {
		return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
	});
	return it != name.end() || filter.empty();
}

std::string MakeComponentTitle(Component *component, const std::string *typeName) {
	if (auto *behaviour = dynamic_cast<MonoBehavior *>(component)) {
		const auto *script = MonoScript::FromMonoBehavior(behaviour);
		return script ? script->GetName() + " (Script)" : "Missing (Mono Script)";
	}

	const Component &componentRef = *component;
	return typeName ? *typeName : std::string(typeid(componentRef).name());
}

} // namespace



ou::InspectorWindow::InspectorWindow() {
	SetTitle("Inspector");
}


void ou::InspectorWindow::OnGui() {
	auto *gameObject = Selection::GetActiveGameObject();
	if (!gameObject) {
		inspectedId_ = 0;
		componentIds_.clear();
		editors_.clear();
		ImGui::TextDisabled("Hierarchy で GameObject を選んでください");
		return;
	}

	RebuildEditorsIfNeeded(gameObject);

	DrawGameObjectHeader(gameObject);
	for (auto &editor : editors_) {
		DrawComponent(editor.get());
	}

	DrawAddComponentButton(gameObject);
}


void ou::InspectorWindow::RebuildEditorsIfNeeded(GameObject *gameObject) {
	std::vector<Component *> components;
	std::vector<uint32_t> componentIds;
	for (auto *component : gameObject->GetComponents<Component>()) {
		if (component->IsDestroyed()) { continue; }
		components.push_back(component);
		componentIds.push_back(component->GetInstanceId());
	}

	if (gameObject->GetInstanceId() == inspectedId_ && componentIds == componentIds_) { return; }

	inspectedId_ = gameObject->GetInstanceId();
	componentIds_ = std::move(componentIds);
	editors_.clear();
	for (auto *component : components) {
		editors_.push_back(Editor::Create(component));
	}
}


void ou::InspectorWindow::DrawGameObjectHeader(GameObject *gameObject) {
	auto isActive = gameObject->IsActiveSelf();
	if (ImGui::Checkbox("##Active", &isActive)) {
		gameObject->SetActive(isActive);
		MarkDirty(gameObject);
	}

	ImGui::SameLine();
	auto name = gameObject->GetName();
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::InputText("##Name", &name)) {
		gameObject->SetName(name);
		MarkDirty(gameObject);
	}

	const auto layer = gameObject->GetLayer();
	const auto layerName = LayerMask::LayerToName(layer);
	const auto preview = layerName.empty() ? std::to_string(layer) : layerName;
	EditorGuiLayout::PrefixLabel("Layer");
	if (ImGui::BeginCombo("##Layer", preview.c_str())) {
		for (int i = 0; i < LayerMask::kLayerCount; i++) {
			const auto candidate = LayerMask::LayerToName(i);
			if (candidate.empty()) { continue; }

			if (ImGui::Selectable(candidate.c_str(), i == layer)) {
				gameObject->SetLayer(i);
				MarkDirty(gameObject);
			}
		}
		ImGui::EndCombo();
	}

	ImGui::Separator();
}


void ou::InspectorWindow::DrawComponent(Editor *editor) {
	auto *component = static_cast<Component *>(editor->GetTarget());
	const auto *typeName = TypeRegistry::FindName(*component);
	const auto title = MakeComponentTitle(component, typeName);

	ImGui::PushID(static_cast<int>(component->GetInstanceId()));
	const auto isOpen = ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
	DrawComponentMenu(component);
	if (isOpen && !component->IsDestroyed()) {
		if (!typeName) {
			ImGui::TextDisabled("型が登録されていないので、シーンに保存されません");
		}

		editor->OnInspectorGui();
	}
	ImGui::PopID();
}


void ou::InspectorWindow::DrawComponentMenu(Component *component) {
	if (!ImGui::BeginPopupContextItem("##ComponentMenu")) { return; }

	const auto canRemove = dynamic_cast<Transform *>(component) == nullptr;
	if (ImGui::MenuItem("Remove Component", nullptr, false, canRemove)) {
		Object::Destroy(component);
		MarkDirty(component->GetGameObject());
	}
	ImGui::EndPopup();
}


void ou::InspectorWindow::DrawAddComponentButton(GameObject *gameObject) {
	ImGui::Spacing();
	if (ImGui::Button("Add Component", {-FLT_MIN, 0.0f})) {
		addComponentFilter_.clear();
		ImGui::OpenPopup("##AddComponent");
	}
	if (!ImGui::BeginPopup("##AddComponent")) { return; }

	// Unity と同じく検索欄を付ける。一覧は長いので、高さを決めて中をスクロールさせる
	if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); }
	ImGui::SetNextItemWidth(260.0f);
	ImGui::InputTextWithHint("##Search", "Search", &addComponentFilter_);

	const auto listHeight = std::min(ImGui::GetTextLineHeightWithSpacing() * 16.0f, ImGui::GetMainViewport()->WorkSize.y * 0.5f);
	if (ImGui::BeginChild("##List", {260.0f, listHeight})) {
		for (const auto &typeName : TypeRegistry::GetTypeNames()) {
			// MonoBehavior はスクリプトから付ける（下の Scripts）
			if (typeName == "Transform" || typeName == "MonoBehavior") { continue; }
			if (!MatchesFilter(typeName, addComponentFilter_)) { continue; }

			if (ImGui::MenuItem(typeName.c_str())) {
				if (auto component = TypeRegistry::Create(typeName)) {
					gameObject->AddComponentInstance(std::move(component));
					MarkDirty(gameObject);
				}
				ImGui::CloseCurrentPopup();
			}
		}

		DrawAddScriptItems(gameObject);
	}
	ImGui::EndChild();
	ImGui::EndPopup();
}


void ou::InspectorWindow::DrawAddScriptItems(GameObject *gameObject) {
	ImGui::SeparatorText("Scripts");

	auto hasScript = false;
	for (const auto &guid : AssetDatabase::FindAssets("t:MonoScript")) {
		// EngineAssets の基底クラスなどは付けられない
		const auto path = AssetDatabase::GuidToAssetPath(guid);
		if (!path.starts_with("Assets/")) { continue; }
		hasScript = true;

		const auto className = WideToUtf8(std::filesystem::path(Utf8ToWide(path)).stem().wstring());
		if (!MatchesFilter(className, addComponentFilter_)) { continue; }

		ImGui::PushID(path.c_str());
		const auto clicked = ImGui::MenuItem(className.c_str());
		ImGui::SetItemTooltip("%s", path.c_str());
		ImGui::PopID();
		if (!clicked) { continue; }

		ImGui::CloseCurrentPopup();
		const auto script = AssetDatabase::LoadAssetAtPath<MonoScript>(path);
		if (!script) { continue; }

		// Play 中に付けたときに Awake に間に合うように、付ける前にスクリプトを入れる
		auto behaviour = std::make_unique<MonoBehavior>();
		behaviour->SetScript(script);
		gameObject->AddComponentInstance(std::move(behaviour));
		MarkDirty(gameObject);
	}

	if (!hasScript) {
		ImGui::TextDisabled("Assets/ に .as がありません");
	}
}
