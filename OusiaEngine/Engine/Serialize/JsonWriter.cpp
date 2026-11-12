#include "JsonWriter.h"
#include "Asset/AssetDatabase.h"
#include "Diagnostics/Log.h"

#include <charconv>

#include <nlohmann/json.hpp>

using namespace ou;


namespace {

using Json = nlohmann::ordered_json;

double ToShortestDouble(float value) {
	char buffer[32];
	const auto written = std::to_chars(buffer, buffer + sizeof(buffer), value);

	double result = value;
	std::from_chars(buffer, written.ptr, result);
	return result;
}

} // namespace



ou::JsonWriter::JsonWriter(nlohmann::ordered_json &target, const std::unordered_set<const Object *> &savedObjects)
	: stack_{&target},
	  savedObjects_(&savedObjects) {}


bool ou::JsonWriter::IsReading() const {
	return false;
}


void ou::JsonWriter::Field(std::string_view name, bool &value) {
	Slot(name) = value;
}


void ou::JsonWriter::Field(std::string_view name, int32_t &value) {
	Slot(name) = value;
}


void ou::JsonWriter::Field(std::string_view name, uint32_t &value) {
	Slot(name) = value;
}


void ou::JsonWriter::Field(std::string_view name, float &value) {
	Slot(name) = ToShortestDouble(value);
}


void ou::JsonWriter::Field(std::string_view name, Vector2 &value) {
	Slot(name) = Json::array(
		{ToShortestDouble(value.x),
		 ToShortestDouble(value.y)});
}


void ou::JsonWriter::Field(std::string_view name, Vector3 &value) {
	Slot(name) = Json::array(
		{ToShortestDouble(value.x),
		 ToShortestDouble(value.y),
		 ToShortestDouble(value.z)});
}


void ou::JsonWriter::Field(std::string_view name, Vector4 &value) {
	Slot(name) = Json::array(
		{ToShortestDouble(value.x),
		 ToShortestDouble(value.y),
		 ToShortestDouble(value.z),
		 ToShortestDouble(value.w)});
}


void ou::JsonWriter::Field(std::string_view name, Quaternion &value) {
	Slot(name) = Json::array(
		{ToShortestDouble(value.x),
		 ToShortestDouble(value.y),
		 ToShortestDouble(value.z),
		 ToShortestDouble(value.w)});
}


void ou::JsonWriter::Field(std::string_view name, Color &value) {
	Slot(name) = Json::array(
		{ToShortestDouble(value.r),
		 ToShortestDouble(value.g),
		 ToShortestDouble(value.b),
		 ToShortestDouble(value.a)});
}


void ou::JsonWriter::Field(std::string_view name, Rect &value) {
	Slot(name) = Json::array(
		{ToShortestDouble(value.x),
		 ToShortestDouble(value.y),
		 ToShortestDouble(value.width),
		 ToShortestDouble(value.height)});
}


void ou::JsonWriter::Field(std::string_view name, std::string &value) {
	Slot(name) = value;
}


bool ou::JsonWriter::BeginObject(std::string_view name) {
	auto &json = Slot(name);
	json = Json::object();

	stack_.push_back(&json);
	return true;
}


void ou::JsonWriter::EndObject() {
	if (stack_.size() > 1) { stack_.pop_back(); }
}


bool ou::JsonWriter::BeginArray(std::string_view name, size_t &) {
	auto &json = Slot(name);
	json = Json::array();

	stack_.push_back(&json);
	return true;
}


void ou::JsonWriter::EndArray() {
	if (stack_.size() > 1) { stack_.pop_back(); }
}


void ou::JsonWriter::ObjectReference(std::string_view name, Object *&value) {
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


void ou::JsonWriter::AssetObject(std::string_view name, std::shared_ptr<Object> &value) {
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


nlohmann::ordered_json &ou::JsonWriter::Slot(std::string_view name) {
	auto &current = *stack_.back();
	if (current.is_array()) { return current.emplace_back(); }

	return current[std::string(name)];
}
