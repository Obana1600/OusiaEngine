#include "EditorGuiLayout.h"
#include "Asset/AssetDatabase.h"
#include "Asset/AssetImporter.h"
#include "Editor/EditorFunc.h"
#include "Editor/EditorPayload.h"
#include "Editor/PrefabUtility.h"
#include "Math/Mathf.h"
#include "Render/Asset/Texture.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <format>
#include <string>
#include <vector>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h> // BeginComboPreview
#include <imgui/imgui_stdlib.h>

using namespace ou;


namespace {

// Inspector の幅の 45% から 40 を引く (最低 120)
float CalcLabelWidth(float viewWidth) {
	return std::max(viewWidth * 0.45f - 40.0f, 120.0f);
}

// 画像は文字の高さにそろえ、横は元の比のまま
ImVec2 CalcImageSize(const Texture &image) {
	const auto height = ImGui::GetFontSize();
	const auto aspect = static_cast<float>(image.GetWidth()) / static_cast<float>(std::max(image.GetHeight(), 1u));
	return {height * aspect, height};
}

// 画像と文字を並べたときの幅
float CalcContentWidth(const GuiContent &content) {
	auto width = ImGui::CalcTextSize(content.text.c_str()).x;
	if (content.image) {
		width += CalcImageSize(*content.image).x;
		if (!content.text.empty()) { width += ImGui::GetStyle().ItemInnerSpacing.x; }
	}
	return width;
}

// 画像を左に、文字をその右に描く。pos は文字の左上
void DrawContent(ImDrawList *drawList, ImVec2 pos, const GuiContent &content) {
	if (content.image) {
		const auto size = CalcImageSize(*content.image);
		drawList->AddImage(ToTextureId(content.image->GetGraphicsTexture()), pos, {pos.x + size.x, pos.y + size.y});
		pos.x += size.x + ImGui::GetStyle().ItemInnerSpacing.x;
	}
	drawList->AddText(pos, ImGui::GetColorU32(ImGuiCol_Text), content.text.c_str());
}

// 直前の項目の中に、縦は真ん中で content を描く。offsetX は項目の左端からのずらし
void DrawContentInLastItem(const GuiContent &content, float offsetX) {
	const auto min = ImGui::GetItemRectMin();
	const auto max = ImGui::GetItemRectMax();
	const ImVec2 pos{min.x + offsetX, min.y + (max.y - min.y - ImGui::GetFontSize()) * 0.5f};

	auto *drawList = ImGui::GetWindowDrawList();
	drawList->PushClipRect(min, max, true);
	DrawContent(drawList, pos, content);
	drawList->PopClipRect();
}

// -0.000 と出ないように、ごく小さい値は 0 にする
float HideNegativeZero(float value) {
	return std::abs(value) < 0.0005f ? 0.0f : value;
}

Vector3 HideNegativeZero(Vector3 value) {
	return {HideNegativeZero(value.x), HideNegativeZero(value.y), HideNegativeZero(value.z)};
}

const char *ChooseFormat(float value) {
	return std::abs(value) >= 1e6f ? "%g" : "%.3f";
}


// ---- シーンの中のものの欄 ----

std::string DescribeObject(const Object *object) {
	if (!object) { return "None"; }
	if (const auto *component = dynamic_cast<const Component *>(object)) {
		return component->GetGameObject()->GetName() + " (Component)";
	}
	if (PrefabUtility::IsPartOfPrefabAsset(object)) { return object->GetName() + " (Prefab)"; }
	return object->GetName();
}

// Hierarchy から落とした GameObject のうち、欄に入れられるもの（GameObject 自身か、そのコンポーネント）
Object *FindAcceptable(GameObject *gameObject, Serializer::ObjectFilter accepts) {
	if (!accepts) { return nullptr; }
	if (accepts(gameObject)) { return gameObject; }
	for (auto *component : gameObject->GetComponents<Component>()) {
		if (accepts(component)) { return component; }
	}
	return nullptr;
}


// ---- アセットの欄 ----

std::string DescribeAsset(const Object *asset, std::string_view typeName) {
	if (!asset) { return std::format("None ({})", typeName); }

	const auto reference = AssetDatabase::FindAssetReference(asset);
	if (!reference) { return asset->GetName() + " (アセットではない)"; }

	std::string name;
	if (reference->subAsset.empty()) {
		const auto path = AssetDatabase::GuidToAssetPath(reference->guid);
		const auto file = path.substr(path.find_last_of('/') + 1);
		name = file.substr(0, file.find_last_of('.'));
	} else {
		name = reference->subAsset.substr(reference->subAsset.find_last_of('/') + 1);
	}
	return std::format("{} ({})", name, typeName);
}

std::string DescribeAssetPath(const Object *asset) {
	const auto reference = AssetDatabase::FindAssetReference(asset);
	if (!reference) { return {}; }

	const auto path = AssetDatabase::GuidToAssetPath(reference->guid);
	return reference->subAsset.empty() ? path : path + " : " + reference->subAsset;
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

// 開いているピッカーは 1 つだけなので、候補は 1 組だけ持つ
struct PickerState {
	std::vector<AssetCandidate> candidates;
	std::string search;
};
PickerState sPicker;

// メインのアセットと、モデルのサブアセットのうち "型/" で始まるもの
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


// ---- フラグの欄 ----

std::string DescribeFlags(std::span<const std::string_view> names, int flags) {
	const auto count = static_cast<int>(std::min<size_t>(names.size(), 31));
	const auto everything = count > 0 ? (1 << count) - 1 : 0;
	if (flags == 0) { return "Nothing"; }
	if ((flags & everything) == everything) { return "Everything"; }

	for (int i = 0; i < count; i++) {
		if (flags == (1 << i)) { return std::string(names[i]); }
	}
	return "Mixed ...";
}

} // namespace



void ou::EditorGuiLayout::PrefixLabel(std::string_view label) {
	PrefixLabel(GuiContent(label));
}


void ou::EditorGuiLayout::PrefixLabel(const GuiContent &content) {
	const auto &style = ImGui::GetStyle();
	const auto startX = ImGui::GetCursorStartPos().x;
	const auto rowX = ImGui::GetCursorPosX();
	const auto viewWidth = ImGui::GetContentRegionAvail().x + (rowX - startX);

	const auto valueX = startX + CalcLabelWidth(viewWidth);
	const auto labelWidth = std::max(valueX - rowX - style.ItemSpacing.x, 1.0f);

	const auto pos = ImGui::GetCursorScreenPos();
	const auto height = ImGui::GetFrameHeight();
	auto *drawList = ImGui::GetWindowDrawList();
	drawList->PushClipRect(pos, {pos.x + labelWidth, pos.y + height}, true);
	DrawContent(drawList, {pos.x, pos.y + style.FramePadding.y}, content);
	drawList->PopClipRect();

	// 切れているときは全部の文字を、ツールチップがあればそれも出す
	ImGui::Dummy({labelWidth, height});
	const auto isCut = CalcContentWidth(content) > labelWidth;
	if ((isCut || !content.tooltip.empty()) && ImGui::IsItemHovered()) {
		if (isCut && !content.tooltip.empty()) {
			ImGui::SetTooltip("%s\n%s", content.text.c_str(), content.tooltip.c_str());
		} else {
			ImGui::SetTooltip("%s", isCut ? content.text.c_str() : content.tooltip.c_str());
		}
	}

	ImGui::SameLine(valueX);
	ImGui::SetNextItemWidth(-FLT_MIN);
}


bool ou::EditorGuiLayout::PropertyField(SerializedProperty property, bool includeChildren) {
	const auto label = property.GetDisplayName();
	return PropertyField(std::move(property), label, includeChildren);
}


bool ou::EditorGuiLayout::PropertyField(SerializedProperty property, std::string_view label, bool includeChildren) {
	return PropertyField(std::move(property), GuiContent(label), includeChildren);
}


bool ou::EditorGuiLayout::PropertyField(SerializedProperty property, const GuiContent &label, bool includeChildren) {
	const auto *schema = property.FindSchema();
	if (!schema) { return false; }

	// 欄の id はパスで分ける（同じ名前の欄が入れ子にあってもぶつからない）
	ImGui::PushID(property.path_.c_str());
	auto isExpanded = false;

	switch (schema->type) {
	case SerializedPropertyType::Boolean: {
		auto value = property.GetBoolValue();
		PrefixLabel(label);
		if (ImGui::Checkbox("##Value", &value)) { property.SetBoolValue(value); }
		break;
	}
	case SerializedPropertyType::Integer: {
		PrefixLabel(label);
		if (schema->isUnsigned) {
			auto value = property.GetUintValue();
			if (ImGui::DragScalar("##Value", ImGuiDataType_U32, &value)) { property.SetUintValue(value); }
		} else {
			auto value = property.GetIntValue();
			if (ImGui::DragInt("##Value", &value)) { property.SetIntValue(value); }
		}
		break;
	}
	case SerializedPropertyType::Float: {
		auto value = HideNegativeZero(property.GetFloatValue());
		PrefixLabel(label);
		if (ImGui::DragFloat("##Value", &value, 0.01f, 0.0f, 0.0f, ChooseFormat(value))) { property.SetFloatValue(value); }
		break;
	}
	case SerializedPropertyType::String: {
		auto value = property.GetStringValue();
		PrefixLabel(label);
		if (ImGui::InputTextMultiline("##Value", &value)) { property.SetStringValue(value); }
		break;
	}
	case SerializedPropertyType::Vector2: {
		auto value = property.GetVector2Value();
		value.x = HideNegativeZero(value.x);
		value.y = HideNegativeZero(value.y);
		PrefixLabel(label);
		if (ImGui::DragFloat2("##Value", &value.x, 0.01f)) { property.SetVector2Value(value); }
		break;
	}
	case SerializedPropertyType::Vector3: {
		auto value = HideNegativeZero(property.GetVector3Value());
		PrefixLabel(label);
		if (ImGui::DragFloat3("##Value", &value.x, 0.01f)) { property.SetVector3Value(value); }
		break;
	}
	case SerializedPropertyType::Vector4: {
		auto value = property.GetVector4Value();
		value.x = HideNegativeZero(value.x);
		value.y = HideNegativeZero(value.y);
		value.z = HideNegativeZero(value.z);
		value.w = HideNegativeZero(value.w);
		PrefixLabel(label);
		if (ImGui::DragFloat4("##Value", &value.x, 0.01f)) { property.SetVector4Value(value); }
		break;
	}
	case SerializedPropertyType::Quaternion: {
		// 度数で見せる。ヒント（ラジアン）があればそれを、なければ Quaternion から求める
		auto hint = property.FindEulerHint();
		const auto radians = hint ? hint->GetVector3Value() : property.GetQuaternionValue().EulerAngles();
		auto degrees = HideNegativeZero(radians * Mathf::Rad2Deg);
		PrefixLabel(label);
		if (ImGui::DragFloat3("##Value", &degrees.x, 0.5f)) {
			const auto changed = degrees * Mathf::Deg2Rad;
			if (hint) { hint->SetVector3Value(changed); }
			property.SetQuaternionValue(Quaternion::Euler(changed));
		}
		break;
	}
	case SerializedPropertyType::Color: {
		auto value = property.GetColorValue();
		PrefixLabel(label);
		if (ImGui::ColorEdit4("##Value", &value.r)) { property.SetColorValue(value); }
		break;
	}
	case SerializedPropertyType::Rect: {
		auto value = property.GetRectValue();
		PrefixLabel(label);
		if (ImGui::DragFloat4("##Value", &value.x, 0.5f)) { property.SetRectValue(value); }
		break;
	}
	case SerializedPropertyType::Enum: {
		const auto names = property.GetEnumNames();
		PrefixLabel(label);

		if (!schema->isFlags) {
			const auto index = property.GetEnumValueIndex();
			const auto preview = index >= 0 ? std::string(names[index]) : property.GetStringValue();
			if (ImGui::BeginCombo("##Value", preview.c_str())) {
				for (int i = 0; i < static_cast<int>(names.size()); i++) {
					if (ImGui::Selectable(std::string(names[i]).c_str(), i == index)) { property.SetEnumValueIndex(i); }
				}
				ImGui::EndCombo();
			}
			break;
		}

		// フラグは Unity と同じく Nothing・Everything と、1 つずつの切り替え
		const auto flags = property.GetEnumValueFlag();
		const auto count = static_cast<int>(std::min<size_t>(names.size(), 31));
		const auto everything = count > 0 ? (1 << count) - 1 : 0;
		if (ImGui::BeginCombo("##Value", DescribeFlags(names, flags).c_str())) {
			ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
			if (ImGui::MenuItem("Nothing", nullptr, flags == 0)) { property.SetEnumValueFlag(0); }
			if (ImGui::MenuItem("Everything", nullptr, (flags & everything) == everything)) { property.SetEnumValueFlag(everything); }
			ImGui::Separator();
			for (int i = 0; i < count; i++) {
				const auto isOn = (flags & (1 << i)) != 0;
				if (ImGui::MenuItem(std::string(names[i]).c_str(), nullptr, isOn)) { property.SetEnumValueFlag(flags ^ (1 << i)); }
			}
			ImGui::PopItemFlag();
			ImGui::EndCombo();
		}
		break;
	}
	case SerializedPropertyType::ObjectReference: {
		auto *value = property.GetObjectReferenceValue();
		PrefixLabel(label);

		if (!schema->isAsset) {
			// シーンの中のもの: Hierarchy から落とす・右クリックで None
			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, {0.0f, 0.5f});
			ImGui::Button((DescribeObject(value) + "##Value").c_str(), {-FLT_MIN, 0.0f});
			ImGui::PopStyleVar();

			if (ImGui::BeginDragDropTarget()) {
				if (const auto *payload = ImGui::AcceptDragDropPayload(EditorPayload::kGameObject)) {
					uint32_t instanceId = 0;
					std::memcpy(&instanceId, payload->Data, sizeof(instanceId));
					if (auto *dropped = dynamic_cast<GameObject *>(Object::FindObjectFromInstanceId(instanceId))) {
						if (auto *found = FindAcceptable(dropped, schema->accepts)) { property.SetObjectReferenceValue(found); }
					}
				}
				// Unity と同じく、GameObject の欄には Project からプレハブも落とせる（入れられないときは落とせない見た目のまま）
				if (const auto *peek = ImGui::AcceptDragDropPayload(EditorPayload::kAsset, ImGuiDragDropFlags_AcceptPeekOnly)) {
					const auto dropped = LoadDroppedAsset(*peek);
					if (PrefabUtility::IsPartOfPrefabAsset(dropped.get()) && property.Accepts(dropped.get()) && ImGui::AcceptDragDropPayload(EditorPayload::kAsset)) {
						property.SetObjectReferenceValue(dropped.get());
					}
				}
				ImGui::EndDragDropTarget();
			}
			if (ImGui::BeginPopupContextItem("##Menu")) {
				if (ImGui::MenuItem("None")) { property.SetObjectReferenceValue(nullptr); }
				ImGui::EndPopup();
			}
			break;
		}

		// アセット: 値のボタン（Project から落とす・右クリックで None）＋ ピッカーを開くボタン
		const auto &style = ImGui::GetStyle();
		const auto pickerWidth = ImGui::GetFrameHeight();
		const auto valueWidth = std::max(ImGui::GetContentRegionAvail().x - pickerWidth - style.ItemInnerSpacing.x, 1.0f);

		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, {0.0f, 0.5f});
		ImGui::Button((DescribeAsset(value, schema->objectTypeName) + "##Value").c_str(), {valueWidth, 0.0f});
		ImGui::PopStyleVar();
		if (value && ImGui::IsItemHovered()) {
			const auto assetPath = DescribeAssetPath(value);
			if (!assetPath.empty()) { ImGui::SetTooltip("%s", assetPath.c_str()); }
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const auto *peek = ImGui::AcceptDragDropPayload(EditorPayload::kAsset, ImGuiDragDropFlags_AcceptPeekOnly)) {
				// 入れられない型のときは、落とせない見た目のままにする
				const auto dropped = LoadDroppedAsset(*peek);
				if (dropped && property.Accepts(dropped.get()) && ImGui::AcceptDragDropPayload(EditorPayload::kAsset)) {
					property.SetObjectReferenceValue(dropped.get());
				}
			}
			ImGui::EndDragDropTarget();
		}
		if (ImGui::BeginPopupContextItem("##Menu")) {
			if (ImGui::MenuItem("None")) { property.SetObjectReferenceValue(nullptr); }
			ImGui::EndPopup();
		}

		ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
		if (ImGui::Button("o##OpenPicker", {pickerWidth, 0.0f})) {
			sPicker = {FindCandidates(schema->objectTypeName), {}};
			ImGui::OpenPopup("##Picker");
		}

		if (ImGui::BeginPopup("##Picker")) {
			ImGui::Text("Select %.*s", static_cast<int>(schema->objectTypeName.size()), schema->objectTypeName.data());
			if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); }
			ImGui::SetNextItemWidth(360.0f);
			ImGui::InputTextWithHint("##Search", "名前で絞る", &sPicker.search);
			ImGui::Separator();

			if (ImGui::Selectable("None", !value)) { property.SetObjectReferenceValue(nullptr); }
			for (const auto &candidate : sPicker.candidates) {
				if (!ContainsIgnoringCase(candidate.label, sPicker.search)) { continue; }
				if (ImGui::Selectable(candidate.label.c_str(), IsCurrentAsset(value, candidate))) {
					const auto picked = AssetDatabase::LoadAsset(AssetDatabase::AssetPathToGuid(candidate.path), candidate.subAsset);
					property.SetObjectReferenceValue(picked.get());
				}
			}
			ImGui::EndPopup();
		}
		break;
	}
	case SerializedPropertyType::Generic: {
		// 入れ子は開いた形、配列は閉じた形で始める（前と同じ）
		ImGuiTreeNodeFlags flags = schema->isArray ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_DefaultOpen;
		if (!includeChildren) { flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen; }

		// 画像があるときは、見出しの文字を空にして、矢印の右に画像と文字を描く
		const auto isOpen = ImGui::TreeNodeEx("##Foldout", flags, "%s", label.image ? "" : label.text.c_str());
		if (label.image) { DrawContentInLastItem(label, ImGui::GetTreeNodeToLabelSpacing()); }
		if (!label.tooltip.empty()) { ImGui::SetItemTooltip("%s", label.tooltip.c_str()); }
		if (!includeChildren) {
			isExpanded = isOpen && property.HasVisibleChildren();
			break;
		}
		if (!isOpen) { break; }

		if (schema->isArray) {
			auto size = property.GetArraySize();
			PrefixLabel("Size");
			if (ImGui::DragInt("##Size", &size, 0.1f, 0, 1024)) { property.SetArraySize(std::max(size, 0)); }
		}

		// 子を順にたどる。深さが自分以下になったら、自分の子は終わり
		const auto depth = property.GetDepth();
		auto child = property;
		for (auto enterChildren = true; child.NextVisible(enterChildren) && child.GetDepth() > depth; enterChildren = false) {
			PropertyField(child, true);
		}
		ImGui::TreePop();
		break;
	}
	}

	ImGui::PopID();
	return isExpanded;
}


int ou::EditorGuiLayout::Popup(const GuiContent &label, int selectedIndex, std::span<const GuiContent> displayedOptions) {
	const auto count = static_cast<int>(displayedOptions.size());
	const auto *current = selectedIndex >= 0 && selectedIndex < count ? &displayedOptions[selectedIndex] : nullptr;

	ImGui::PushID(label.text.c_str());
	PrefixLabel(label);

	const auto hasImage = current && current->image;
	const auto isOpen = ImGui::BeginCombo("##Popup", current && !hasImage ? current->text.c_str() : "", hasImage ? ImGuiComboFlags_CustomPreview : ImGuiComboFlags_None);

	auto result = selectedIndex;
	if (isOpen) {
		for (int i = 0; i < count; i++) {
			const auto &option = displayedOptions[i];
			ImGui::PushID(i);
			if (ImGui::Selectable(option.image ? "##Option" : option.text.c_str(), i == selectedIndex)) { result = i; }
			if (option.image) { DrawContentInLastItem(option, 0.0f); }
			if (!displayedOptions[i].tooltip.empty()) { ImGui::SetItemTooltip("%s", displayedOptions[i].tooltip.c_str()); }
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}

	// 今の値に画像があるときは、閉じた欄の中を自分で描く（ImGui では BeginCombo / EndCombo のあとに呼ぶ）
	if (hasImage && ImGui::BeginComboPreview()) {
		DrawContent(ImGui::GetWindowDrawList(), ImGui::GetCursorScreenPos(), *current);
		ImGui::EndComboPreview();
	}

	// 閉じているときは、今の値の説明を出す
	if (!isOpen && current && !current->tooltip.empty()) { ImGui::SetItemTooltip("%s", current->tooltip.c_str()); }

	ImGui::PopID();
	return result;
}
