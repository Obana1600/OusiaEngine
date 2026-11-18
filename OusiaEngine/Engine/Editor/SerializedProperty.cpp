#include "SerializedProperty.h"
#include "Asset/AssetDatabase.h"
#include "Core/JsonFile.h"
#include "Editor/SerializedObject.h"

#include <algorithm>
#include <cctype>

using namespace ou;


ou::SerializedProperty::SerializedProperty(SerializedObject *object, std::string path)
	: object_(object),
	  path_(std::move(path)) {}


SerializedObject *ou::SerializedProperty::GetSerializedObject() const {
	return object_;
}


std::string ou::SerializedProperty::GetPropertyPath() const {
	return path_.empty() ? std::string() : path_.substr(1);
}


const std::string &ou::SerializedProperty::GetName() const {
	static const std::string kEmpty;
	const auto *schema = FindSchema();
	return schema ? schema->name : kEmpty;
}


std::string ou::SerializedProperty::GetDisplayName() const {
	std::string_view name = GetName();

	// Unity と同じく m_・_・k の前置きを外す
	if (name.starts_with("m_")) {
		name.remove_prefix(2);
	} else if (name.starts_with("_")) {
		name.remove_prefix(1);
	} else if (name.size() > 1 && name[0] == 'k' && std::isupper(static_cast<unsigned char>(name[1]))) {
		name.remove_prefix(1);
	}

	// 単語の頭の大文字の前に空白を入れ、最初を大文字にする（"localPosition" → "Local Position"、"UV" はそのまま）
	std::string display;
	for (size_t i = 0; i < name.size(); i++) {
		const auto c = static_cast<unsigned char>(name[i]);
		const auto previous = i > 0 ? static_cast<unsigned char>(name[i - 1]) : 0;
		const auto next = i + 1 < name.size() ? static_cast<unsigned char>(name[i + 1]) : 0;
		const auto startsWord = i > 0 && std::isupper(c) && (std::islower(previous) || (std::isupper(previous) && std::islower(next)));
		if (startsWord) { display += ' '; }
		display += static_cast<char>(i == 0 ? std::toupper(c) : c);
	}
	return display;
}


SerializedPropertyType ou::SerializedProperty::GetPropertyType() const {
	const auto *schema = FindSchema();
	return schema ? schema->type : SerializedPropertyType::Generic;
}


int ou::SerializedProperty::GetDepth() const {
	const auto *schema = FindSchema();
	return schema ? schema->depth : -1;
}


bool ou::SerializedProperty::NextVisible(bool enterChildren) {
	const auto &schema = object_->schema_;

	size_t index = 0;
	if (!path_.empty()) {
		const auto it = object_->schemaIndices_.find(path_);
		if (it == object_->schemaIndices_.end()) { return false; }

		const auto depth = schema[it->second].depth;
		index = it->second + 1;
		if (!enterChildren) {
			while (index < schema.size() && schema[index].depth > depth) { index++; }
		}
	}

	while (index < schema.size() && !schema[index].isVisible) { index++; }
	if (index >= schema.size()) { return false; }

	path_ = schema[index].path;
	return true;
}


bool ou::SerializedProperty::HasVisibleChildren() const {
	const auto it = object_->schemaIndices_.find(path_);
	if (it == object_->schemaIndices_.end()) { return false; }

	const auto &schema = object_->schema_;
	const auto depth = schema[it->second].depth;
	for (auto index = it->second + 1; index < schema.size() && schema[index].depth > depth; index++) {
		if (schema[index].isVisible) { return true; }
	}
	return false;
}


std::optional<SerializedProperty> ou::SerializedProperty::FindPropertyRelative(std::string_view relativePath) const {
	return object_->FindProperty(GetPropertyPath() + "/" + std::string(relativePath));
}


bool ou::SerializedProperty::IsArray() const {
	const auto *schema = FindSchema();
	return schema && schema->isArray;
}


int ou::SerializedProperty::GetArraySize() const {
	const auto *value = FindValue();
	return value && value->is_array() ? static_cast<int>(value->size()) : 0;
}


void ou::SerializedProperty::SetArraySize(int size) {
	const auto *value = FindValue();
	if (!value || !value->is_array() || size < 0) { return; }

	auto array = *value;
	const auto last = array.empty() ? Json() : array.back();
	while (static_cast<int>(array.size()) < size) { array.push_back(last); }
	while (static_cast<int>(array.size()) > size) { array.erase(array.size() - 1); }
	Write(std::move(array));
}


std::optional<SerializedProperty> ou::SerializedProperty::GetArrayElementAtIndex(int index) const {
	if (!IsArray() || index < 0) { return std::nullopt; }
	return object_->FindProperty(GetPropertyPath() + "/" + std::to_string(index));
}


bool ou::SerializedProperty::GetBoolValue() const {
	const auto *value = FindValue();
	return value && value->is_boolean() && value->get<bool>();
}


void ou::SerializedProperty::SetBoolValue(bool value) {
	Write(value);
}


int ou::SerializedProperty::GetIntValue() const {
	const auto *value = FindValue();
	return value && value->is_number_integer() ? value->get<int>() : 0;
}


void ou::SerializedProperty::SetIntValue(int value) {
	Write(value);
}


uint32_t ou::SerializedProperty::GetUintValue() const {
	const auto *value = FindValue();
	return value && value->is_number_unsigned() ? value->get<uint32_t>() : 0;
}


void ou::SerializedProperty::SetUintValue(uint32_t value) {
	Write(value);
}


float ou::SerializedProperty::GetFloatValue() const {
	const auto *value = FindValue();
	return value && value->is_number() ? value->get<float>() : 0.0f;
}


void ou::SerializedProperty::SetFloatValue(float value) {
	Write(value);
}


std::string ou::SerializedProperty::GetStringValue() const {
	const auto *value = FindValue();
	return value && value->is_string() ? value->get<std::string>() : std::string();
}


void ou::SerializedProperty::SetStringValue(std::string_view value) {
	Write(std::string(value));
}


Vector2 ou::SerializedProperty::GetVector2Value() const {
	float values[2] = {};
	ReadFloats(values);
	return {values[0], values[1]};
}


void ou::SerializedProperty::SetVector2Value(const Vector2 &value) {
	WriteFloats({value.x, value.y});
}


Vector3 ou::SerializedProperty::GetVector3Value() const {
	float values[3] = {};
	ReadFloats(values);
	return {values[0], values[1], values[2]};
}


void ou::SerializedProperty::SetVector3Value(const Vector3 &value) {
	WriteFloats({value.x, value.y, value.z});
}


Vector4 ou::SerializedProperty::GetVector4Value() const {
	float values[4] = {};
	ReadFloats(values);
	return {values[0], values[1], values[2], values[3]};
}


void ou::SerializedProperty::SetVector4Value(const Vector4 &value) {
	WriteFloats({value.x, value.y, value.z, value.w});
}


Quaternion ou::SerializedProperty::GetQuaternionValue() const {
	float values[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	ReadFloats(values);
	return {values[0], values[1], values[2], values[3]};
}


void ou::SerializedProperty::SetQuaternionValue(const Quaternion &value) {
	WriteFloats({value.x, value.y, value.z, value.w});
}


Color ou::SerializedProperty::GetColorValue() const {
	float values[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	ReadFloats(values);
	return {values[0], values[1], values[2], values[3]};
}


void ou::SerializedProperty::SetColorValue(const Color &value) {
	WriteFloats({value.r, value.g, value.b, value.a});
}


Rect ou::SerializedProperty::GetRectValue() const {
	float values[4] = {};
	ReadFloats(values);
	return {values[0], values[1], values[2], values[3]};
}


void ou::SerializedProperty::SetRectValue(const Rect &value) {
	WriteFloats({value.x, value.y, value.width, value.height});
}


Object *ou::SerializedProperty::GetObjectReferenceValue() const {
	const auto *value = FindValue();
	if (!value || !value->is_object()) { return nullptr; }

	// シーンの中のもの: {"id": fileId}
	if (const auto id = value->find("id"); id != value->end() && id->is_number_unsigned()) {
		const auto &objectsById = object_->sceneObjects_.objectsById;
		const auto it = objectsById.find(id->get<uint64_t>());
		return it != objectsById.end() ? it->second : nullptr;
	}

	// アセット: {"guid": "...", "sub": "..."}
	const auto guidJson = value->find("guid");
	if (guidJson == value->end() || !guidJson->is_string()) { return nullptr; }
	const auto guid = Guid::Parse(guidJson->get<std::string>());
	if (!guid) { return nullptr; }

	const auto sub = value->find("sub");
	const auto subAsset = sub != value->end() && sub->is_string() ? sub->get<std::string>() : std::string();
	return AssetDatabase::LoadAsset(*guid, subAsset).get(); // AssetDatabase が持ち続けるので、生のポインタで返してよい
}


void ou::SerializedProperty::SetObjectReferenceValue(Object *value) {
	if (!FindValue()) { return; }
	if (!value) {
		Write(nullptr);
		return;
	}
	if (!Accepts(value)) { return; }

	if (FindSchema()->isAsset) {
		const auto reference = AssetDatabase::FindAssetReference(value);
		if (!reference) { return; }

		auto json = Json::object();
		json["guid"] = reference->guid.ToString();
		if (!reference->subAsset.empty()) { json["sub"] = reference->subAsset; }
		Write(std::move(json));
		return;
	}

	// シーンの中のものは、保存されるもの（同じシーン）だけ
	if (!object_->sceneObjects_.savedObjects.contains(value)) { return; }
	auto json = Json::object();
	json["id"] = value->GetFileId();
	Write(std::move(json));
}


std::span<const std::string_view> ou::SerializedProperty::GetEnumNames() const {
	const auto *schema = FindSchema();
	return schema ? schema->enumNames : std::span<const std::string_view>();
}


int ou::SerializedProperty::GetEnumValueIndex() const {
	const auto names = GetEnumNames();
	const auto text = GetStringValue();
	const auto it = std::find(names.begin(), names.end(), text);
	return it != names.end() ? static_cast<int>(it - names.begin()) : -1;
}


void ou::SerializedProperty::SetEnumValueIndex(int index) {
	const auto names = GetEnumNames();
	if (index < 0 || index >= static_cast<int>(names.size())) { return; }
	Write(std::string(names[index]));
}


int ou::SerializedProperty::GetEnumValueFlag() const {
	const auto names = GetEnumNames();
	const auto text = GetStringValue();
	const std::string_view view = text;

	int flags = 0;
	size_t begin = 0;
	while (begin < view.size()) {
		const auto end = std::min(view.find('|', begin), view.size());
		const auto it = std::find(names.begin(), names.end(), view.substr(begin, end - begin));
		if (it != names.end() && it - names.begin() < 31) { flags |= 1 << (it - names.begin()); }
		begin = end + 1;
	}
	return flags;
}


void ou::SerializedProperty::SetEnumValueFlag(int flags) {
	const auto names = GetEnumNames();

	std::string text;
	for (size_t i = 0; i < names.size() && i < 31; i++) {
		if (!(flags & (1 << i))) { continue; }
		if (!text.empty()) { text += '|'; }
		text += names[i];
	}
	Write(std::move(text));
}


const PropertySchema *ou::SerializedProperty::FindSchema() const {
	return object_ ? object_->FindSchema(path_) : nullptr;
}


const Json *ou::SerializedProperty::FindValue() const {
	if (!object_ || path_.empty()) { return nullptr; }

	const Json::json_pointer pointer(path_);
	return object_->json_.contains(pointer) ? &object_->json_.at(pointer) : nullptr;
}


void ou::SerializedProperty::Write(Json value) {
	if (!FindValue()) { return; }

	object_->json_.at(Json::json_pointer(path_)) = std::move(value);
	object_->MarkModified(path_);
}


template <size_t N>
bool ou::SerializedProperty::ReadFloats(float (&out)[N]) const {
	const auto *value = FindValue();
	if (!value || !value->is_array() || value->size() != N) { return false; }

	for (size_t i = 0; i < N; i++) {
		if (!(*value)[i].is_number()) { return false; }
	}
	for (size_t i = 0; i < N; i++) {
		out[i] = (*value)[i].get<float>();
	}
	return true;
}


template <size_t N>
void ou::SerializedProperty::WriteFloats(const float (&values)[N]) {
	auto array = Json::array();
	for (const auto value : values) {
		array.push_back(value);
	}
	Write(std::move(array));
}


bool ou::SerializedProperty::Accepts(const Object *value) const {
	const auto *schema = FindSchema();
	return schema && schema->type == SerializedPropertyType::ObjectReference && schema->accepts && schema->accepts(value);
}


std::optional<SerializedProperty> ou::SerializedProperty::FindEulerHint() const {
	const auto *schema = FindSchema();
	if (!schema || schema->hintPath.empty()) { return std::nullopt; }
	return SerializedProperty(object_, schema->hintPath);
}
