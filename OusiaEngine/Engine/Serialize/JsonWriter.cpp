#include "JsonWriter.h"
#include "Asset/AssetDatabase.h"
#include "Core/JsonFile.h"
#include "Diagnostics/Log.h"

#include <charconv>
#include <format>

using namespace ou;


namespace {

double ToShortestDouble(float value) {
	char buffer[32];
	const auto written = std::to_chars(buffer, buffer + sizeof(buffer), value);

	double result = value;
	std::from_chars(buffer, written.ptr, result);
	return result;
}

} // namespace



ou::JsonWriter::JsonWriter(Json &target, const std::unordered_set<const Object *> &savedObjects)
	: stack_{&target},
	  savedObjects_(&savedObjects),
	  paths_{std::string()} {}


void ou::JsonWriter::SetSchemaOutput(std::vector<PropertySchema> *schema) {
	schema_ = schema;
}


bool ou::JsonWriter::IsReading() const {
	return false;
}


void ou::JsonWriter::Field(std::string_view name, bool &value) {
	Record(name, SerializedPropertyType::Boolean);
	Slot(name) = value;
}


void ou::JsonWriter::Field(std::string_view name, int32_t &value) {
	Record(name, SerializedPropertyType::Integer);
	Slot(name) = value;
}


void ou::JsonWriter::Field(std::string_view name, uint32_t &value) {
	if (auto *property = Record(name, SerializedPropertyType::Integer)) { property->isUnsigned = true; }
	Slot(name) = value;
}


void ou::JsonWriter::Field(std::string_view name, float &value) {
	Record(name, SerializedPropertyType::Float);
	Slot(name) = ToShortestDouble(value);
}


void ou::JsonWriter::Field(std::string_view name, Vector2 &value) {
	Record(name, SerializedPropertyType::Vector2);
	Slot(name) = Json::array(
		{ToShortestDouble(value.x),
		 ToShortestDouble(value.y)});
}


void ou::JsonWriter::Field(std::string_view name, Vector3 &value) {
	Record(name, SerializedPropertyType::Vector3);
	Slot(name) = Json::array(
		{ToShortestDouble(value.x),
		 ToShortestDouble(value.y),
		 ToShortestDouble(value.z)});
}


void ou::JsonWriter::Field(std::string_view name, Vector4 &value) {
	Record(name, SerializedPropertyType::Vector4);
	Slot(name) = Json::array(
		{ToShortestDouble(value.x),
		 ToShortestDouble(value.y),
		 ToShortestDouble(value.z),
		 ToShortestDouble(value.w)});
}


void ou::JsonWriter::Field(std::string_view name, Quaternion &value) {
	Record(name, SerializedPropertyType::Quaternion);
	Slot(name) = Json::array(
		{ToShortestDouble(value.x),
		 ToShortestDouble(value.y),
		 ToShortestDouble(value.z),
		 ToShortestDouble(value.w)});
}


void ou::JsonWriter::Field(std::string_view name, Color &value) {
	Record(name, SerializedPropertyType::Color);
	Slot(name) = Json::array(
		{ToShortestDouble(value.r),
		 ToShortestDouble(value.g),
		 ToShortestDouble(value.b),
		 ToShortestDouble(value.a)});
}


void ou::JsonWriter::Field(std::string_view name, Rect &value) {
	Record(name, SerializedPropertyType::Rect);
	Slot(name) = Json::array(
		{ToShortestDouble(value.x),
		 ToShortestDouble(value.y),
		 ToShortestDouble(value.width),
		 ToShortestDouble(value.height)});
}


void ou::JsonWriter::Field(std::string_view name, std::string &value) {
	Record(name, SerializedPropertyType::String);
	Slot(name) = value;
}


void ou::JsonWriter::RotationField(std::string_view name, Quaternion &rotation, std::string_view hintName, Vector3 &eulerHint) {
	Field(name, rotation);
	const auto rotationIndex = schema_ ? schema_->size() - 1 : 0;
	Field(hintName, eulerHint);

	if (schema_) {
		(*schema_)[rotationIndex].hintPath = schema_->back().path;
		schema_->back().isVisible = false;
	}
}


bool ou::JsonWriter::BeginObject(std::string_view name) {
	const auto *property = Record(name, SerializedPropertyType::Generic);
	paths_.push_back(property ? property->path : std::string());

	auto &json = Slot(name);
	json = Json::object();

	stack_.push_back(&json);
	return true;
}


void ou::JsonWriter::EndObject() {
	if (stack_.size() > 1) {
		stack_.pop_back();
		paths_.pop_back();
	}
}


bool ou::JsonWriter::BeginArray(std::string_view name, size_t &) {
	auto *property = Record(name, SerializedPropertyType::Generic);
	if (property) { property->isArray = true; }
	paths_.push_back(property ? property->path : std::string());

	auto &json = Slot(name);
	json = Json::array();

	stack_.push_back(&json);
	return true;
}


void ou::JsonWriter::EndArray() {
	if (stack_.size() > 1) {
		stack_.pop_back();
		paths_.pop_back();
	}
}


Json *ou::JsonWriter::FindWritingJson() {
	return stack_.empty() ? nullptr : stack_.back();
}


void ou::JsonWriter::ObjectReference(std::string_view name, Object *&value, ObjectFilter accepts) {
	if (auto *property = Record(name, SerializedPropertyType::ObjectReference)) { property->accepts = accepts; }
	auto &json = Slot(name);

	if (!value) {
		json = nullptr;
		return;
	}

	if (!savedObjects_->contains(value)) {
		Log::Warning("保存されないオブジェクトを参照しています: \"{}\" -> {}", name, value->GetName());
		json = nullptr;
		return;
	}

	json = Json::object();
	json["id"] = value->GetFileId();
}


void ou::JsonWriter::AssetObject(std::string_view name, std::shared_ptr<Object> &value, const AssetType &type) {
	if (auto *property = Record(name, SerializedPropertyType::ObjectReference)) {
		property->isAsset = true;
		property->accepts = type.accepts;
		property->objectTypeName = type.name;
	}
	auto &json = Slot(name);

	if (!value) {
		json = nullptr;
		return;
	}

	const auto reference = AssetDatabase::FindAssetReference(value.get());
	if (!reference) {
		Log::Warning("アセットではないオブジェクトは保存できません: \"{}\" -> {}", name, value->GetName());
		json = nullptr;
		return;
	}

	json = Json::object();
	json["guid"] = reference->guid.ToString();
	if (!reference->subAsset.empty()) {
		json["sub"] = reference->subAsset;
	}
}


void ou::JsonWriter::EnumField(std::string_view name, std::string &text, std::span<const std::string_view> names) {
	if (auto *property = Record(name, SerializedPropertyType::Enum)) { property->enumNames = names; }
	Slot(name) = text;
}


void ou::JsonWriter::FlagsField(std::string_view name, std::string &text, std::span<const std::string_view> names) {
	if (auto *property = Record(name, SerializedPropertyType::Enum)) {
		property->enumNames = names;
		property->isFlags = true;
	}
	Slot(name) = text;
}


PropertySchema *ou::JsonWriter::Record(std::string_view name, SerializedPropertyType type) {
	if (!schema_) { return nullptr; }

	const auto &current = *stack_.back();
	auto &property = schema_->emplace_back();
	property.path = MakeChildPath(name);
	property.name = current.is_array() ? std::format("Element {}", current.size()) : std::string(name);
	property.type = type;
	property.depth = static_cast<int>(stack_.size()) - 1;
	return &property;
}


std::string ou::JsonWriter::MakeChildPath(std::string_view name) const {
	const auto &current = *stack_.back();

	if (current.is_array()) { return paths_.back() + "/" + std::to_string(current.size()); }

	auto path = paths_.back() + "/";
	for (const auto c : name) {
		if (c == '~') {
			path += "~0";
		} else if (c == '/') {
			path += "~1";
		} else {
			path += c;
		}
	}
	return path;
}


Json &ou::JsonWriter::Slot(std::string_view name) {
	auto &current = *stack_.back();
	if (current.is_array()) { return current.emplace_back(); }

	return current[std::string(name)];
}
