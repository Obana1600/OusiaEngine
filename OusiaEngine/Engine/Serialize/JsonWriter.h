#pragma once

#include "Core/JsonFile.h"
#include "Serialize/PropertySchema.h"
#include "Serialize/Serializer.h"

#include <string>
#include <unordered_set>
#include <vector>


namespace ou {

class JsonWriter final: public Serializer {
public: // メンバ関数

	JsonWriter(Json &target, const std::unordered_set<const Object *> &savedObjects);

	void SetSchemaOutput(std::vector<PropertySchema> *schema);

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

	void RotationField(std::string_view name, Quaternion &rotation, std::string_view hintName, Vector3 &eulerHint) override;

	bool BeginObject(std::string_view name) override;
	void EndObject() override;
	bool BeginArray(std::string_view name, size_t &count) override;
	void EndArray() override;

	Json *FindWritingJson() override;


private: // メンバ変数

	std::vector<Json *> stack_;
	const std::unordered_set<const Object *> *savedObjects_ = nullptr;

	std::vector<PropertySchema> *schema_ = nullptr;
	std::vector<std::string> paths_;


private: // メンバ関数

	void ObjectReference(std::string_view name, Object *&value, ObjectFilter accepts) override;
	void AssetObject(std::string_view name, std::shared_ptr<Object> &value, const AssetType &type) override;

	void EnumField(std::string_view name, std::string &text, std::span<const std::string_view> names) override;
	void FlagsField(std::string_view name, std::string &text, std::span<const std::string_view> names) override;

	PropertySchema *Record(std::string_view name, SerializedPropertyType type);
	std::string MakeChildPath(std::string_view name) const;

	Json &Slot(std::string_view name);
};

} // namespace ou
