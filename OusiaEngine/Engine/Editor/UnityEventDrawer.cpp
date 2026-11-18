#include "UnityEventDrawer.h"
#include "Editor/EditorGuiLayout.h"
#include "Editor/EditorGuiUtility.h"
#include "Editor/SerializedObject.h"
#include "Events/UnityEvent.h"
#include "Scene/GameObject.h"
#include "Script/MonoBehavior.h"

#include <format>
#include <string>

#include <angelscript.h>
#include <imgui/imgui.h>

using namespace ou;


namespace {

// 引数の型の見せ方（Unity の一覧と同じく、メソッド名の後ろに付ける）
std::string_view DescribeMode(PersistentListenerMode mode) {
	switch (mode) {
	case PersistentListenerMode::Int: return "int";
	case PersistentListenerMode::Float: return "float";
	case PersistentListenerMode::String: return "string";
	case PersistentListenerMode::Bool: return "bool";
	default: return "";
	}
}

// 引数の欄の名前（mode と同じ順）
std::string_view ArgumentPath(PersistentListenerMode mode) {
	switch (mode) {
	case PersistentListenerMode::Int: return "intArgument";
	case PersistentListenerMode::Float: return "floatArgument";
	case PersistentListenerMode::String: return "stringArgument";
	case PersistentListenerMode::Bool: return "boolArgument";
	default: return "";
	}
}

PersistentListenerMode ReadMode(SerializedObject &so, const std::string &elementPath) {
	const auto property = so.FindProperty(elementPath + "/mode");
	const auto index = property ? property->GetEnumValueIndex() : 0;
	return index > 0 ? static_cast<PersistentListenerMode>(index) : PersistentListenerMode::Void;
}

std::string ReadString(SerializedObject &so, const std::string &path) {
	const auto property = so.FindProperty(path);
	return property ? property->GetStringValue() : std::string();
}

} // namespace



void ou::UnityEventDrawer::Draw(SerializedObject &so, std::string_view path, std::string_view label) {
	auto array = so.FindProperty(path);
	if (!array) { return; }

	ImGui::PushID(std::string(path).c_str());
	ImGui::SeparatorText(std::string(label).c_str());

	const auto count = array->GetArraySize();
	if (count == 0) { ImGui::TextDisabled("つないだものはありません"); }

	for (int i = 0; i < count; i++) {
		ImGui::PushID(i);
		auto isRemoved = false;
		DrawCall(so, std::format("{}/{}", path, i), isRemoved);
		ImGui::PopID();

		if (isRemoved) {
			RemoveCall(so, std::string(path), i);
			break;
		}
		ImGui::Separator();
	}

	// Unity の + ボタン。増やすと最後の要素が写される（同じ対象に別のメソッドをつなぐことが多いので、そのままにする）
	if (ImGui::Button("+ つなぐものを足す")) { array->SetArraySize(count + 1); }
	ImGui::PopID();
}


void ou::UnityEventDrawer::DrawCall(SerializedObject &so, const std::string &elementPath, bool &isRemoved) {
	// 足した直後は、次の Update まで中の欄がない
	auto target = so.FindProperty(elementPath + "/target");
	if (!target) {
		ImGui::TextDisabled("…");
		return;
	}

	EditorGuiLayout::PropertyField(*target, EditorGuiUtility::TrTextContent("対象", "target（スクリプトを付けた GameObject を Hierarchy から落とす）"));
	DrawMethodPopup(so, elementPath);

	const auto mode = ReadMode(so, elementPath);
	if (mode != PersistentListenerMode::Void) {
		if (auto argument = so.FindProperty(elementPath + "/" + std::string(ArgumentPath(mode)))) {
			EditorGuiLayout::PropertyField(*argument, EditorGuiUtility::TrTextContent("渡す値", "argument"));
		}
	}

	if (ImGui::SmallButton("外す")) { isRemoved = true; }
}


void ou::UnityEventDrawer::DrawMethodPopup(SerializedObject &so, const std::string &elementPath) {
	auto target = so.FindProperty(elementPath + "/target");
	auto methodName = so.FindProperty(elementPath + "/methodName");
	auto mode = so.FindProperty(elementPath + "/mode");
	if (!target || !methodName || !mode) { return; }

	auto *behavior = dynamic_cast<MonoBehavior *>(target->GetObjectReferenceValue());
	const auto currentName = methodName->GetStringValue();
	const auto currentMode = ReadMode(so, elementPath);

	auto preview = std::string("No Function");
	if (behavior && !currentName.empty()) {
		preview = std::format("{}.{}({})", FindScriptClassName(behavior), currentName, DescribeMode(currentMode));
	} else if (!currentName.empty()) {
		preview = std::format("{}（対象がありません）", currentName);
	}

	EditorGuiLayout::PrefixLabel(EditorGuiUtility::TrTextContent("メソッド", "methodName（void で、引数が 0 か 1 つ（int・float・string・bool）のもの）"));
	ImGui::BeginDisabled(!behavior);
	if (ImGui::BeginCombo("##Method", preview.c_str())) {
		if (ImGui::Selectable("No Function", currentName.empty())) {
			methodName->SetStringValue("");
			mode->SetEnumValueIndex(0);
		}

		// Unity と同じく、同じ GameObject のスクリプトのメソッドを全部並べる（選ぶと、対象もそのスクリプトになる）
		for (auto *candidate : behavior->GetGameObject()->GetComponents<MonoBehavior>()) {
			const auto methods = candidate->FindListenerMethods();
			if (methods.empty()) { continue; }

			ImGui::SeparatorText(FindScriptClassName(candidate).c_str());
			for (const auto &method : methods) {
				const auto isSelected = candidate == behavior && method.name == currentName && method.mode == currentMode;
				const auto text = std::format("{}({})##{}", method.name, DescribeMode(method.mode), static_cast<const void *>(candidate));
				if (ImGui::Selectable(text.c_str(), isSelected)) {
					target->SetObjectReferenceValue(candidate);
					methodName->SetStringValue(method.name);
					mode->SetEnumValueIndex(static_cast<int>(method.mode));
				}
			}
		}
		ImGui::EndCombo();
	}
	ImGui::EndDisabled();
}


void ou::UnityEventDrawer::RemoveCall(SerializedObject &so, const std::string &path, int index) {
	auto array = so.FindProperty(path);
	if (!array) { return; }

	// 後ろの要素を 1 つずつ前へ写してから、最後を消す
	const auto count = array->GetArraySize();
	for (int i = index; i + 1 < count; i++) {
		const auto to = std::format("{}/{}/", path, i);
		const auto from = std::format("{}/{}/", path, i + 1);

		if (auto target = so.FindProperty(to + "target")) {
			const auto source = so.FindProperty(from + "target");
			target->SetObjectReferenceValue(source ? source->GetObjectReferenceValue() : nullptr);
		}
		if (auto property = so.FindProperty(to + "methodName")) { property->SetStringValue(ReadString(so, from + "methodName")); }
		if (auto property = so.FindProperty(to + "mode")) { property->SetEnumValueIndex(static_cast<int>(ReadMode(so, from.substr(0, from.size() - 1)))); }
		if (auto property = so.FindProperty(to + "intArgument")) {
			const auto source = so.FindProperty(from + "intArgument");
			property->SetIntValue(source ? source->GetIntValue() : 0);
		}
		if (auto property = so.FindProperty(to + "floatArgument")) {
			const auto source = so.FindProperty(from + "floatArgument");
			property->SetFloatValue(source ? source->GetFloatValue() : 0.0f);
		}
		if (auto property = so.FindProperty(to + "stringArgument")) { property->SetStringValue(ReadString(so, from + "stringArgument")); }
		if (auto property = so.FindProperty(to + "boolArgument")) {
			const auto source = so.FindProperty(from + "boolArgument");
			property->SetBoolValue(source ? source->GetBoolValue() : false);
		}
	}
	array->SetArraySize(count - 1);
}


std::string ou::UnityEventDrawer::FindScriptClassName(const MonoBehavior *behavior) {
	return behavior->instance_ ? behavior->instance_->GetObjectType()->GetName() : std::string("MonoBehavior");
}
