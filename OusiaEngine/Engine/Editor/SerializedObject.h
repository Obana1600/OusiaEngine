#pragma once

#include "Core/JsonFile.h"
#include "Editor/SerializedProperty.h"
#include "Serialize/PropertySchema.h"
#include "Serialize/SceneSerializer.h"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>


namespace ou {

// 前方宣言
class Object;

class SerializedObject final {
	friend class SerializedProperty;

public: // メンバ関数

	explicit SerializedObject(Object *target);

	Object *GetTargetObject() const;

	void Update();

	bool ApplyModifiedProperties();
	bool HasModifiedProperties() const;

	std::optional<SerializedProperty> FindProperty(std::string_view propertyPath);

	SerializedProperty GetIterator();


private: // メンバ変数

	Object *target_ = nullptr;

	Json json_;
	std::vector<PropertySchema> schema_;
	std::unordered_map<std::string, size_t> schemaIndices_;

	SceneObjects sceneObjects_;
	std::vector<std::string> modifiedKeys_;


private: // メンバ関数

	const PropertySchema *FindSchema(const std::string &path) const;
	void MarkModified(std::string_view path);
};

} // namespace ou
