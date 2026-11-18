#include "InspectorWindow.h"
#include "Asset/AssetDatabase.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/Selection.h"
#include "Editor/Undo.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"
#include "Scene/LayerMask.h"
#include "Scene/Transform.h"
#include "Script/MonoBehavior.h"
#include "Script/MonoScript.h"
#include "Serialize/TypeRegistry.h"
#include "Core/StringUtil.h"
#include "Editor/EditorGuiLayout.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <typeinfo>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

using namespace ou;


namespace {

void MarkDirty(GameObject *gameObject, std::string_view undoName) {
	Undo::SetCurrentGroupName(undoName);
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

// MSVC の typeid の名前（"class ou::Texture"）から、型の名前（"Texture"）だけにする
std::string MakeTypeName(const Object &object) {
	std::string name = typeid(object).name();
	for (const std::string_view prefix : {"class ", "struct "}) {
		if (name.starts_with(prefix)) { name.erase(0, prefix.size()); }
	}
	if (const auto colon = name.rfind("::"); colon != std::string::npos) { name.erase(0, colon + 2); }
	return name;
}

} // namespace



ou::InspectorWindow::InspectorWindow() {
	SetTitle("Inspector");
}


void ou::InspectorWindow::OnGui() {
	// Unity と同じく、選んでいるのが GameObject ならコンポーネントを並べ、アセットならそのアセットの Editor を 1 つ出す
	if (auto *gameObject = Selection::GetActiveGameObject()) {
		ClearAssetEditor();
		DrawGameObjectInspector(gameObject);
		return;
	}

	ClearGameObjectEditors();
	if (Selection::GetActiveAssetPath().empty()) {
		ClearAssetEditor();
		ImGui::TextDisabled("Hierarchy で GameObject を、Project でアセットを選んでください");
		return;
	}
	DrawAssetInspector();
}


void ou::InspectorWindow::DrawGameObjectInspector(GameObject *gameObject) {
	RebuildEditorsIfNeeded(gameObject);

	DrawGameObjectHeader(gameObject);
	for (auto &editor : editors_) {
		DrawComponent(editor.get());
	}

	DrawAddComponentButton(gameObject);
}


void ou::InspectorWindow::ClearGameObjectEditors() {
	inspectedId_ = 0;
	componentIds_.clear();
	editors_.clear();
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


void ou::InspectorWindow::DrawAssetInspector() {
	auto *asset = Selection::GetActiveObject();
	RebuildAssetEditorIfNeeded(asset);
	DrawAssetHeader(asset);

	if (!asset) {
		if (!isFolder_) { ImGui::TextDisabled("このファイルは Inspector に出せません"); }
		return;
	}

	// TODO: プレハブは Unity と同じく、ルートのコンポーネントを出して Open Prefab で開けるようにする
	if (dynamic_cast<GameObject *>(asset)) {
		ImGui::TextDisabled("プレハブの中身は、まだ Inspector で編集できません");
		return;
	}

	auto *editor = assetEditor_.get();
	const auto hasPreview = editor && editor->HasPreviewGui();

	// Preview があるときは、その分を下に空けて、上の欄はスクロールさせる
	const auto previewTotal = hasPreview ? (isPreviewOpen_ ? previewHeight_ : 0.0f) + ImGui::GetFrameHeightWithSpacing() + 6.0f : 0.0f;
	const auto bodyHeight = hasPreview ? std::max(ImGui::GetContentRegionAvail().y - previewTotal, ImGui::GetFrameHeight()) : 0.0f;
	if (ImGui::BeginChild("##AssetBody", {0.0f, bodyHeight})) {
		// 専用の Editor がない型は、値を見せるだけにする（ファイルへ書き戻す仕組みがまだないので、変えても保存されない）
		// TODO: Material などは専用の Editor を作り、変えたらファイルへ書き戻す
		const auto isReadOnly = editor && typeid(*editor) == typeid(Editor);
		ImGui::BeginDisabled(isReadOnly);
		if (editor) { editor->OnInspectorGui(); }
		ImGui::EndDisabled();
	}
	ImGui::EndChild();

	if (hasPreview) { DrawPreview(editor); }
}


void ou::InspectorWindow::RebuildAssetEditorIfNeeded(Object *asset) {
	const auto &assetPath = Selection::GetActiveAssetPath();
	const auto &subAsset = Selection::GetActiveSubAsset();
	const auto assetId = asset ? asset->GetInstanceId() : 0;
	if (assetPath == inspectedAssetPath_ && subAsset == inspectedSubAsset_ && assetId == inspectedAssetId_) { return; }

	inspectedAssetPath_ = assetPath;
	inspectedSubAsset_ = subAsset;
	inspectedAssetId_ = assetId;

	// フォルダかは選び直したときだけ調べる（毎フレーム、ファイルを見に行かない）
	std::error_code error;
	const auto fullPath = AssetDatabase::AssetPathToFullPath(assetPath);
	isFolder_ = !asset && fullPath && std::filesystem::is_directory(*fullPath, error);

	// インポートし直したときも、別のオブジェクトになるので作り直す
	assetEditor_ = (asset && !dynamic_cast<GameObject *>(asset)) ? Editor::Create(asset) : nullptr;
}


void ou::InspectorWindow::ClearAssetEditor() {
	inspectedAssetPath_.clear();
	inspectedSubAsset_.clear();
	inspectedAssetId_ = 0;
	isFolder_ = false;
	assetEditor_.reset();
}


void ou::InspectorWindow::DrawAssetHeader(Object *asset) {
	// 名前はファイル名（サブアセットならその名前）。型はその下に小さく出す（Unity の「Texture2D Import Settings」の行）
	const auto &assetPath = Selection::GetActiveAssetPath();
	const auto &subAsset = Selection::GetActiveSubAsset();
	const auto fileName = WideToUtf8(std::filesystem::path(Utf8ToWide(assetPath)).filename().wstring());
	const auto &name = subAsset.empty() ? fileName : subAsset;

	ImGui::TextUnformatted(name.c_str());
	const auto typeName = asset ? MakeTypeName(*asset) : std::string(isFolder_ ? "Folder" : "File");
	ImGui::TextDisabled("%s  %s", typeName.c_str(), assetPath.c_str());
	ImGui::SetItemTooltip("%s%s%s", assetPath.c_str(), subAsset.empty() ? "" : " : ", subAsset.c_str());
	ImGui::Separator();
}


void ou::InspectorWindow::DrawPreview(Editor *editor) {
	// 境目。ドラッグで Preview の高さを変える
	ImGui::InvisibleButton("##PreviewSplitter", {-FLT_MIN, 6.0f});
	if (ImGui::IsItemHovered() || ImGui::IsItemActive()) { ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS); }
	if (ImGui::IsItemActive() && isPreviewOpen_) {
		previewHeight_ -= ImGui::GetIO().MouseDelta.y;
	}
	{
		const auto min = ImGui::GetItemRectMin();
		const auto max = ImGui::GetItemRectMax();
		const auto y = (min.y + max.y) * 0.5f;
		ImGui::GetWindowDrawList()->AddLine({min.x, y}, {max.x, y}, ImGui::GetColorU32(ImGuiCol_Separator));
	}

	// 見出し（クリックで開け閉め）と、Editor の設定のボタン
	const auto title = editor->GetPreviewTitle();
	if (ImGui::ArrowButton("##PreviewToggle", isPreviewOpen_ ? ImGuiDir_Down : ImGuiDir_Right)) {
		isPreviewOpen_ = !isPreviewOpen_;
	}
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(title.empty() ? "Preview" : title.c_str());
	if (ImGui::IsItemClicked()) { isPreviewOpen_ = !isPreviewOpen_; }
	ImGui::SameLine();
	ImGui::PushID("##PreviewSettings");
	editor->OnPreviewSettings();
	ImGui::PopID();

	if (!isPreviewOpen_) { return; }

	// 高さは、上の欄を少しは残す
	const auto available = ImGui::GetContentRegionAvail();
	previewHeight_ = std::clamp(previewHeight_, 40.0f, std::max(ImGui::GetWindowHeight() - 120.0f, 40.0f));

	constexpr auto kFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(32, 32, 32, 255));
	if (ImGui::BeginChild("##Preview", {0.0f, std::max(available.y, 1.0f)}, ImGuiChildFlags_None, kFlags)) {
		const auto pos = ImGui::GetCursorScreenPos();
		const auto size = ImGui::GetContentRegionAvail();
		ImGui::GetWindowDrawList()->PushClipRect(pos, {pos.x + size.x, pos.y + size.y}, true);
		editor->OnPreviewGui(Rect(pos.x, pos.y, size.x, size.y));
		ImGui::GetWindowDrawList()->PopClipRect();
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();
}


void ou::InspectorWindow::DrawGameObjectHeader(GameObject *gameObject) {
	auto isActive = gameObject->IsActiveSelf();
	if (ImGui::Checkbox("##Active", &isActive)) {
		gameObject->SetActive(isActive);
		MarkDirty(gameObject, "Set Active");
	}

	ImGui::SameLine();
	auto name = gameObject->GetName();
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::InputText("##Name", &name)) {
		gameObject->SetName(name);
		MarkDirty(gameObject, "Rename");
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
				MarkDirty(gameObject, "Change Layer");
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
		MarkDirty(component->GetGameObject(), "Remove Component");
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
					MarkDirty(gameObject, "Add Component");
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
		MarkDirty(gameObject, "Add Component");
	}

	if (!hasScript) {
		ImGui::TextDisabled("Assets/ に .as がありません");
	}
}
