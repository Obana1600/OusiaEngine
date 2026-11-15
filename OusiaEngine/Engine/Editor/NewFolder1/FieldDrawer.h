#pragma once

#include "Editor/NewFolder1/InspectorField.h"
#include "Editor/NewFolder1/PathSerializer.h"

#include <optional>


namespace ou {

class FieldDrawer final: public PathSerializer {
public: // メンバ関数

	const std::optional<FieldChange> &GetChange() const;

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


protected: // メンバ関数

	void ObjectReference(std::string_view name, Object *&value, ObjectFilter accepts) override;
	void AssetObject(std::string_view name, std::shared_ptr<Object> &value, const AssetType &type) override;

	void EnumField(std::string_view name, std::string &text, std::span<const std::string_view> names) override;
	void FlagsField(std::string_view name, std::string &text, std::span<const std::string_view> names) override;


private: // メンバ変数

	std::optional<FieldChange> change_;


private: // メンバ関数

	void Record(const std::string &path, FieldValue value);

	void DrawPicker(const std::string &pickerId, const std::string &path, const std::shared_ptr<Object> &value, const AssetType &type);
};

} // namespace ou
