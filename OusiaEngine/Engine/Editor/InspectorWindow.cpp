#include "InspectorWindow.h"
#include "Asset/AssetDatabase.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/NewFolder1/ChangeApplier.h"
#include "Editor/NewFolder1/FieldDrawer.h"
#include "Editor/Selection.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"
#include "Scene/LayerMask.h"
#include "Scene/Transform.h"
#include "Serialize/TypeRegistry.h"
#include "Editor/EditorGUILayout.h"

#include <cfloat>
#include <optional>
#include <string>
#include <typeinfo>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

using namespace ou;


namespace {

void MarkDirty(GameObject *gameObject) {
	EditorSceneManager::MarkSceneDirty(gameObject->GetScene());
}

} // namespace



ou::InspectorWindow::InspectorWindow() {
	SetTitle("Inspector");
}


void ou::InspectorWindow::OnGui() {
	auto *gameObject = Selection::GetActiveGameObject();
	if (!gameObject) {
		ImGui::TextDisabled("Hierarchy で GameObject を選んでください");
		return;
	}

	DrawGameObjectHeader(gameObject);
	for (auto *component : gameObject->GetComponents<Component>()) {
		if (component->IsDestroyed()) { continue; }
		DrawComponent(component);
	}

	DrawAddComponentButton(gameObject);
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
	EditorGUILayout::PrefixLabel("Layer");
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


void ou::InspectorWindow::DrawComponent(Component *component) {
	const Component &componentRef = *component;
	const auto *typeName = TypeRegistry::FindName(componentRef);
	const auto title = typeName ? *typeName : std::string(typeid(componentRef).name());

	ImGui::PushID(static_cast<int>(component->GetInstanceId()));
	const auto isOpen = ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
	DrawComponentMenu(component);
	if (isOpen) {
		if (!typeName) {
			ImGui::TextDisabled("型が登録されていないので、シーンに保存されません");
		}

		FieldDrawer drawer;
		component->Serialize(drawer);

		if (const auto &change = drawer.GetChange()) {
			ChangeApplier applier(*change);
			component->Serialize(applier);
			MarkDirty(component->GetGameObject());
		}
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
		ImGui::OpenPopup("##AddComponent");
	}
	if (!ImGui::BeginPopup("##AddComponent")) { return; }

	for (const auto &typeName : TypeRegistry::GetTypeNames()) {
		if (typeName == "Transform") { continue; }

		if (ImGui::MenuItem(typeName.c_str())) {
			if (auto component = TypeRegistry::Create(typeName)) {
				gameObject->AddComponentInstance(std::move(component));
				MarkDirty(gameObject);
			}
		}
	}
	ImGui::EndPopup();
}
