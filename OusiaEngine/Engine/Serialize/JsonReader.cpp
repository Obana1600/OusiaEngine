#include "JsonReader.h"
#include "Asset/AssetDatabase.h"
#include "Core/JsonFile.h"
#include "Diagnostics/Log.h"

#include <cstdint>
#include <optional>
#include <string>

using namespace ou;


namespace {

void WarnWrongType(std::string_view name) {
	Log::Warning("値の型が違います: \"{}\"", name);
}

std::optional<AssetDatabase::AssetReference> ParseAssetReference(const Json &json) {
	if (!json.is_object()) { return std::nullopt; }

	const auto guidIt = json.find("guid");
	if (guidIt == json.end() || !guidIt->is_string()) { return std::nullopt; }
	const auto guid = Guid::Parse(guidIt->get<std::string>());
	if (!guid) { return std::nullopt; }

	AssetDatabase::AssetReference reference{*guid, {}};
	if (const auto subIt = json.find("sub"); subIt != json.end()) {
		if (!subIt->is_string()) { return std::nullopt; }
		reference.subAsset = subIt->get<std::string>();
	}
	return reference;
}

} // namespace



ou::JsonReader::JsonReader(const Json &source, const std::unordered_map<uint64_t, Object *> &objectsById)
	: stack_{Frame(&source, 0)},
	  objectsById_(&objectsById) {}


bool ou::JsonReader::IsReading() const {
	return true;
}


void ou::JsonReader::Field(std::string_view name, bool &value) {
	const auto *json = FindValue(name);
	if (!json) { return; }
	if (!json->is_boolean()) { return WarnWrongType(name); }

	value = json->get<bool>();
}


void ou::JsonReader::Field(std::string_view name, int32_t &value) {
	const auto *json = FindValue(name);
	if (!json) { return; }
	if (!json->is_number_integer()) { return WarnWrongType(name); }

	value = json->get<int32_t>();
}


void ou::JsonReader::Field(std::string_view name, uint32_t &value) {
	const auto *json = FindValue(name);
	if (!json) { return; }
	if (!json->is_number_unsigned() || json->get<uint64_t>() > UINT32_MAX) { return WarnWrongType(name); }

	value = json->get<uint32_t>();
}


void ou::JsonReader::Field(std::string_view name, float &value) {
	ReadFloats(name, &value, 1);
}


void ou::JsonReader::Field(std::string_view name, Vector2 &value) {
	float values[2];
	if (ReadFloats(name, values, 2)) { value = {values[0], values[1]}; }
}


void ou::JsonReader::Field(std::string_view name, Vector3 &value) {
	float values[3];
	if (ReadFloats(name, values, 3)) { value = {values[0], values[1], values[2]}; }
}


void ou::JsonReader::Field(std::string_view name, Vector4 &value) {
	float values[4];
	if (ReadFloats(name, values, 4)) { value = {values[0], values[1], values[2], values[3]}; }
}


void ou::JsonReader::Field(std::string_view name, Quaternion &value) {
	float values[4];
	if (ReadFloats(name, values, 4)) { value = {values[0], values[1], values[2], values[3]}; }
}


void ou::JsonReader::Field(std::string_view name, Color &value) {
	float values[4];
	if (ReadFloats(name, values, 4)) { value = {values[0], values[1], values[2], values[3]}; }
}


void ou::JsonReader::Field(std::string_view name, Rect &value) {
	float values[4];
	if (ReadFloats(name, values, 4)) { value = {values[0], values[1], values[2], values[3]}; }
}


void ou::JsonReader::Field(std::string_view name, std::string &value) {
	const auto *json = FindValue(name);
	if (!json) { return; }
	if (!json->is_string()) { return WarnWrongType(name); }

	value = json->get<std::string>();
}


bool ou::JsonReader::BeginObject(std::string_view name) {
	const auto *json = FindValue(name);
	if (!json) { return false; }
	if (!json->is_object()) {
		WarnWrongType(name);
		return false;
	}

	stack_.push_back({json, 0});
	return true;
}


void ou::JsonReader::EndObject() {
	if (stack_.size() > 1) { stack_.pop_back(); }
}


bool ou::JsonReader::BeginArray(std::string_view name, size_t &count) {
	const auto *json = FindValue(name);
	if (!json) { return false; }
	if (!json->is_array()) {
		WarnWrongType(name);
		return false;
	}

	count = json->size();
	stack_.push_back({json, 0});
	return true;
}


void ou::JsonReader::EndArray() {
	if (stack_.size() > 1) { stack_.pop_back(); }
}


const Json *ou::JsonReader::FindReadingJson() const {
	return stack_.empty() ? nullptr : stack_.back().node;
}


void ou::JsonReader::ObjectReference(std::string_view name, Object *&value, ObjectFilter accepts) {
	const auto *json = FindValue(name);
	if (!json) { return; }

	// null チェック
	if (json->is_null()) {
		value = nullptr;
		return;
	}

	// プレハブのようなアセットは guid で書かれている
	if (json->is_object() && json->contains("guid")) {
		const auto reference = ParseAssetReference(*json);
		const auto asset = reference ? AssetDatabase::LoadAsset(reference->guid, reference->subAsset) : nullptr;
		if (asset && accepts && !accepts(asset.get())) {
			value = nullptr;
			return WarnWrongType(name);
		}
		value = asset.get();
		return;
	}

	// Object::fileId
	const auto id = json->is_object() ? json->find("id") : json->end();
	if (id == json->end() || !id->is_number_unsigned()) {
		value = nullptr;
		return WarnWrongType(name);
	}

	const auto it = objectsById_->find(id->get<uint64_t>());
	if (it == objectsById_->end()) {
		Log::Warning("参照先が見つかりません: \"{}\" -> {}", name, id->get<uint64_t>());
		value = nullptr;
		return;
	}

	value = it->second;
}


void ou::JsonReader::AssetObject(std::string_view name, std::shared_ptr<Object> &value, const AssetType &) {
	const auto *json = FindValue(name);
	if (!json) { return; }

	if (json->is_null()) {
		value = nullptr;
		return;
	}

	const auto reference = ParseAssetReference(*json);
	if (!reference) {
		value = nullptr;
		return WarnWrongType(name);
	}

	value = AssetDatabase::LoadAsset(reference->guid, reference->subAsset);
}


const Json *ou::JsonReader::FindValue(std::string_view name) {
	auto &frame = stack_.back();

	if (frame.node->is_array()) {
		if (frame.next >= frame.node->size()) { return nullptr; }
		return &(*frame.node)[frame.next++];
	}

	if (!frame.node->is_object()) { return nullptr; }

	const auto it = frame.node->find(std::string(name));
	return it != frame.node->end() ? &*it : nullptr;
}


bool ou::JsonReader::ReadFloats(std::string_view name, float *out, size_t count) {
	const auto *json = FindValue(name);
	if (!json) { return false; }

	// float 1つ
	if (count == 1) {
		if (!json->is_number()) {
			WarnWrongType(name);
			return false;
		}
		out[0] = json->get<float>();
		return true;
	}

	// float 2つ以上かを調べる
	if (!json->is_array() || json->size() != count) {
		WarnWrongType(name);
		return false;
	}
	for (const auto &element : *json) {
		if (!element.is_number()) {
			WarnWrongType(name);
			return false;
		}
	}

	// float 2つ以上
	for (size_t i = 0; i < count; i++) {
		out[i] = (*json)[i].get<float>();
	}
	return true;
}
