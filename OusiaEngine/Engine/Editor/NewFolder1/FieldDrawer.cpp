#include "FieldDrawer.h"
#include "Asset/AssetDatabase.h"
#include "Editor/EditorPayload.h"
#include "Math/Mathf.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"
#include "Asset/AssetImporter.h"

#include <algorithm>
#include <cctype>
#include <vector>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

using namespace ou;


namespace {

std::string MakeShownName(std::string_view name, const std::string &path) {
	return name.empty() ? path.substr(path.find_last_of('/') + 1) : std::string(name);
}

std::string MakeLabel(std::string_view name, const std::string &path) {
	return MakeShownName(name, path) + "##" + path;
}

std::string DescribeObject(const Object *object) {
	if (!object) { return "None"; }
	if (const auto *component = dynamic_cast<const Component *>(object)) {
		return component->GetGameObject()->GetName() + " (Component)";
	}
	return object->GetName();
}

std::string DescribeAsset(const Object *asset) {
	if (!asset) { return "None"; }

	const auto reference = AssetDatabase::FindAssetReference(asset);
	if (!reference) { return asset->GetName() + " (アセットではない)"; }

	const auto path = AssetDatabase::GuidToAssetPath(reference->guid);
	return reference->subAsset.empty() ? path : path + " : " + reference->subAsset;
}

float HideNegativeZero(float value) {
	return value == 0.0f ? 0.0f : value;
}

const char *ChooseFormat(float value) {
	return std::abs(value) >= 1e6f ? "%g" : "%.3f";
}

Object *FindAcceptable(GameObject *gameObject, Serializer::ObjectFilter accepts) {
	if (accepts(gameObject)) { return gameObject; }
	for (auto *component : gameObject->GetComponents<Component>()) {
		if (accepts(component)) { return component; }
	}
	return nullptr;
}

std::shared_ptr<Object> LoadDroppedAsset(const ImGuiPayload &payload) {
	const auto asset = EditorPayload::DecodeAsset(payload.Data, payload.DataSize);
	return AssetDatabase::LoadAsset(AssetDatabase::AssetPathToGuid(asset.path), asset.subAsset);
}

struct AssetCandidate {
	std::string path;
	std::string subAsset;
	std::string label;
};

struct PickerState {
	std::string fieldPath;
	std::vector<AssetCandidate> candidates;
	std::string search;
};
PickerState sPicker;

std::vector<AssetCandidate> FindCandidates(std::string_view typeName) {
	std::vector<AssetCandidate> candidates;
	for (const auto &guid : AssetDatabase::FindAssets("t:" + std::string(typeName))) {
		const auto path = AssetDatabase::GuidToAssetPath(guid);
		candidates.push_back({path, {}, path});
	}

	const auto prefix = std::string(typeName) + "/";
	for (const auto &guid : AssetDatabase::FindAssets("t:Model")) {
		const auto path = AssetDatabase::GuidToAssetPath(guid);
		if (!AssetDatabase::LoadMainAssetAtPath(path)) { continue; }
		const auto *importer = AssetDatabase::GetImporterAtPath(path);
		if (!importer) { continue; }

		for (const auto &sub : importer->GetSubAssets()) {
			if (sub.name.starts_with(prefix)) {
				candidates.push_back({path, sub.name, path + " : " + sub.name});
			}
		}
	}
	return candidates;
}

bool ContainsIgnoringCase(std::string_view text, std::string_view part) {
	const auto it = std::search(text.begin(), text.end(), part.begin(), part.end(), [](char a, char b) {
		return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
	});
	return it != text.end();
}

bool IsCurrentAsset(const Object *value, const AssetCandidate &candidate) {
	const auto reference = AssetDatabase::FindAssetReference(value);
	return reference && reference->guid == AssetDatabase::AssetPathToGuid(candidate.path) && reference->subAsset == candidate.subAsset;
}

} // namespace



const std::optional<FieldChange> &ou::FieldDrawer::GetChange() const {
	return change_;
}


bool ou::FieldDrawer::IsReading() const {
	return false;
}


void ou::FieldDrawer::Field(std::string_view name, bool &value) {
	const auto path = MakePath(name);
	auto copy = value;
	if (ImGui::Checkbox(MakeLabel(name, path).c_str(), &copy)) { Record(path, copy); }
}


void ou::FieldDrawer::Field(std::string_view name, int32_t &value) {
	const auto path = MakePath(name);
	auto copy = value;
	if (ImGui::DragInt(MakeLabel(name, path).c_str(), &copy)) { Record(path, copy); }
}


void ou::FieldDrawer::Field(std::string_view name, uint32_t &value) {
	const auto path = MakePath(name);
	auto copy = value;
	if (ImGui::DragScalar(MakeLabel(name, path).c_str(), ImGuiDataType_U32, &copy)) { Record(path, copy); }
}


void ou::FieldDrawer::Field(std::string_view name, float &value) {
	const auto path = MakePath(name);
	auto copy = HideNegativeZero(value);
	if (ImGui::DragFloat(MakeLabel(name, path).c_str(), &copy, 0.01f, 0.0f, 0.0f, ChooseFormat(copy))) { Record(path, copy); }
}


void ou::FieldDrawer::Field(std::string_view name, Vector2 &value) {
	const auto path = MakePath(name);
	auto copy = value;
	copy.x = HideNegativeZero(copy.x);
	copy.y = HideNegativeZero(copy.y);
	if (ImGui::DragFloat2(MakeLabel(name, path).c_str(), &copy.x, 0.01f)) { Record(path, copy); }
}


void ou::FieldDrawer::Field(std::string_view name, Vector3 &value) {
	const auto path = MakePath(name);
	auto copy = value;
	copy.x = HideNegativeZero(copy.x);
	copy.y = HideNegativeZero(copy.y);
	copy.z = HideNegativeZero(copy.z);
	if (ImGui::DragFloat3(MakeLabel(name, path).c_str(), &copy.x, 0.01f)) { Record(path, copy); }
}


void ou::FieldDrawer::Field(std::string_view name, Vector4 &value) {
	const auto path = MakePath(name);
	auto copy = value;
	copy.x = HideNegativeZero(copy.x);
	copy.y = HideNegativeZero(copy.y);
	copy.z = HideNegativeZero(copy.z);
	copy.w = HideNegativeZero(copy.w);
	if (ImGui::DragFloat4(MakeLabel(name, path).c_str(), &copy.x, 0.01f)) { Record(path, copy); }
}


void ou::FieldDrawer::Field(std::string_view name, Quaternion &value) {
	const auto path = MakePath(name);
	auto degrees = value.EulerAngles() * Mathf::Rad2Deg;
	degrees.x = HideNegativeZero(degrees.x);
	degrees.y = HideNegativeZero(degrees.y);
	degrees.z = HideNegativeZero(degrees.z);
	if (ImGui::DragFloat3(MakeLabel(name, path).c_str(), &degrees.x, 0.5f)) {
		Record(path, Quaternion::Euler(degrees * Mathf::Deg2Rad));
	}
}


void ou::FieldDrawer::Field(std::string_view name, Color &value) {
	const auto path = MakePath(name);
	auto copy = value;
	if (ImGui::ColorEdit4(MakeLabel(name, path).c_str(), &copy.r)) { Record(path, copy); }
}


void ou::FieldDrawer::Field(std::string_view name, Rect &value) {
	const auto path = MakePath(name);
	auto copy = value;
	if (ImGui::DragFloat4(MakeLabel(name, path).c_str(), &copy.x, 0.5f)) { Record(path, copy); }
}


void ou::FieldDrawer::Field(std::string_view name, std::string &value) {
	const auto path = MakePath(name);
	auto copy = value;
	if (ImGui::InputText(MakeLabel(name, path).c_str(), &copy)) { Record(path, copy); }
}


void ou::FieldDrawer::RotationField(std::string_view name, Quaternion &, std::string_view, Vector3 &eulerHint) {
	const auto path = MakePath(name);

	auto degrees = eulerHint * Mathf::Rad2Deg;
	degrees.x = HideNegativeZero(degrees.x);
	degrees.y = HideNegativeZero(degrees.y);
	degrees.z = HideNegativeZero(degrees.z);
	if (ImGui::DragFloat3(MakeLabel(name, path).c_str(), &degrees.x, 0.5f)) {
		Record(path, degrees * Mathf::Deg2Rad);
	}
}


bool ou::FieldDrawer::BeginObject(std::string_view name) {
	const auto path = MakePath(name);
	if (!ImGui::TreeNodeEx(MakeLabel(name, path).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) { return false; }

	PushFrame(path, false);
	return true;
}


void ou::FieldDrawer::EndObject() {
	PopFrame();
	ImGui::TreePop();
}


bool ou::FieldDrawer::BeginArray(std::string_view name, size_t &count) {
	const auto path = MakePath(name);
	if (!ImGui::TreeNodeEx(MakeLabel(name, path).c_str())) { return false; }

	auto size = static_cast<int>(count);
	if (ImGui::DragInt(("Size##" + path).c_str(), &size, 0.1f, 0, 1024)) {
		Record(path, static_cast<size_t>(std::max(size, 0)));
	}

	PushFrame(path, true);
	return true;
}


void ou::FieldDrawer::EndArray() {
	PopFrame();
	ImGui::TreePop();
}


void ou::FieldDrawer::ObjectReference(std::string_view name, Object *&value, ObjectFilter accepts) {
	const auto path = MakePath(name);
	ImGui::LabelText(MakeLabel(name, path).c_str(), "%s", DescribeObject(value).c_str());

	if (ImGui::BeginDragDropTarget()) {
		if (const auto *payload = ImGui::AcceptDragDropPayload(EditorPayload::kGameObject)) {
			uint32_t instanceId = 0;
			std::memcpy(&instanceId, payload->Data, sizeof(instanceId));
			if (auto *dropped = dynamic_cast<GameObject *>(Object::FindObjectFromInstanceId(instanceId))) {
				if (auto *found = FindAcceptable(dropped, accepts)) {
					Record(path, FieldValue(std::in_place_type<Object *>, found));
				}
			}
		}
		ImGui::EndDragDropTarget();
	}

	// 右クリックで外す
	if (ImGui::BeginPopupContextItem(("##Reference" + path).c_str())) {
		if (ImGui::MenuItem("None")) {
			Record(path, FieldValue(std::in_place_type<Object *>, nullptr));
		}
		ImGui::EndPopup();
	}
}


void ou::FieldDrawer::AssetObject(std::string_view name, std::shared_ptr<Object> &value, const AssetType &type) {
	const auto path = MakePath(name);
	const auto &style = ImGui::GetStyle();
	const auto pickerWidth = ImGui::GetFrameHeight();
	const auto valueWidth = std::max(ImGui::CalcItemWidth() - pickerWidth - style.ItemInnerSpacing.x, 1.0f);

	ImGui::Button((DescribeAsset(value.get()) + "##Value" + path).c_str(), {valueWidth, 0.0f});

	if (ImGui::BeginDragDropTarget()) {
		if (const auto *peek = ImGui::AcceptDragDropPayload(EditorPayload::kAsset, ImGuiDragDropFlags_AcceptPeekOnly)) {
			auto dropped = LoadDroppedAsset(*peek);
			if (dropped && type.accepts(dropped.get()) && ImGui::AcceptDragDropPayload(EditorPayload::kAsset)) {
				Record(path, FieldValue(std::in_place_type<std::shared_ptr<Object>>, std::move(dropped)));
			}
		}
		ImGui::EndDragDropTarget();
	}

	if (ImGui::BeginPopupContextItem(("##Asset" + path).c_str())) {
		if (ImGui::MenuItem("None")) {
			Record(path, FieldValue(std::in_place_type<std::shared_ptr<Object>>, nullptr));
		}
		ImGui::EndPopup();
	}

	const auto pickerId = "##Picker" + path;
	ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
	if (ImGui::Button(("o##OpenPicker" + path).c_str(), {pickerWidth, 0.0f})) {
		sPicker = {path, FindCandidates(type.name), {}};
		ImGui::OpenPopup(pickerId.c_str());
	}

	ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
	ImGui::TextUnformatted(MakeShownName(name, path).c_str());

	DrawPicker(pickerId, path, value, type);
}


void ou::FieldDrawer::EnumField(std::string_view name, std::string &text, std::span<const std::string_view> names) {
	const auto path = MakePath(name);
	if (!ImGui::BeginCombo(MakeLabel(name, path).c_str(), text.c_str())) { return; }

	for (const auto candidate : names) {
		if (ImGui::Selectable(std::string(candidate).c_str(), candidate == text)) {
			Record(path, std::string(candidate));
		}
	}
	ImGui::EndCombo();
}


void ou::FieldDrawer::Record(const std::string &path, FieldValue value) {
	change_ = FieldChange{path, std::move(value)};
}


void ou::FieldDrawer::DrawPicker(const std::string &pickerId, const std::string &path, const std::shared_ptr<Object> &value, const AssetType &type) {
	if (!ImGui::BeginPopup(pickerId.c_str())) { return; }

	ImGui::Text("Select %.*s", static_cast<int>(type.name.size()), type.name.data());
	if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); }
	ImGui::SetNextItemWidth(360.0f);
	ImGui::InputTextWithHint("##Search", "名前で絞る", &sPicker.search);
	ImGui::Separator();

	if (ImGui::Selectable("None", !value)) {
		Record(path, FieldValue(std::in_place_type<std::shared_ptr<Object>>, nullptr));
	}

	for (const auto &candidate : sPicker.candidates) {
		if (!ContainsIgnoringCase(candidate.label, sPicker.search)) { continue; }

		if (ImGui::Selectable(candidate.label.c_str(), IsCurrentAsset(value.get(), candidate))) {
			auto picked = AssetDatabase::LoadAsset(AssetDatabase::AssetPathToGuid(candidate.path), candidate.subAsset);
			if (picked && type.accepts(picked.get())) {
				Record(path, FieldValue(std::in_place_type<std::shared_ptr<Object>>, std::move(picked)));
			}
		}
	}

	ImGui::EndPopup();
}
