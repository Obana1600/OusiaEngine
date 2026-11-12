#pragma once

#include "Serialize/Serializer.h"

#include <unordered_set>
#include <vector>

#include <nlohmann/json_fwd.hpp>


namespace ou {

class JsonWriter final: public Serializer {
public: // メンバ関数

	JsonWriter(nlohmann::ordered_json &target, const std::unordered_set<const Object *> &savedObjects);

	bool IsReading() const override;

	void Field(std::string_view name, bool &value) override;
	void Field(std::string_view name, int32_t &value) override;
	void Field(std::string_view name, uint32_t &value) override;
	void Field(std::string_view name, float &value) override;
	void Field(std::string_view name, Vector2 &value) override;
	void Field(std::string_view name, Vector3 &value) override;
	void Field(std::string_view name, Vector4 &value) override;
	void Field(std::string_view name, Quaternion &value) override;
	void Field(std::string_view name, Color &value) override;
	void Field(std::string_view name, Rect &value) override;
	void Field(std::string_view name, std::string &value) override;

	bool BeginObject(std::string_view name) override;
	void EndObject() override;
	bool BeginArray(std::string_view name, size_t &count) override;
	void EndArray() override;


private: // メンバ変数

	std::vector<nlohmann::ordered_json *> stack_;
	const std::unordered_set<const Object *> *savedObjects_ = nullptr;


private: // メンバ関数

	void ObjectReference(std::string_view name, Object *&value) override;
	void AssetObject(std::string_view name, std::shared_ptr<Object> &value) override;

	nlohmann::ordered_json &Slot(std::string_view name);
};

} // namespace ou
