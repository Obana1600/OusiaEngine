#pragma once

#include "Core/JsonFile.h"
#include "Math/Color.h"
#include "Math/Quaternion.h"
#include "Math/Rect.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Serialize/PropertySchema.h"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>


namespace ou {

// 前方宣言
class Object;
class SerializedObject;

// Unity の SerializedProperty。SerializedObject の JSON の 1 つの値を指す
// 値を変えると SerializedObject の JSON が変わり、ApplyModifiedProperties で Object に戻る
class SerializedProperty final {
	friend class SerializedObject;
	friend class EditorGuiLayout;

public: // メンバ関数

	SerializedObject *GetSerializedObject() const;

	// "localPosition"・"materials/0" のように、名前を / でつないだもの（FindProperty に渡せる）
	std::string GetPropertyPath() const;
	const std::string &GetName() const;
	// "localPosition" → "Local Position"（Unity の ObjectNames.NicifyVariableName）
	std::string GetDisplayName() const;
	SerializedPropertyType GetPropertyType() const;
	int GetDepth() const;

	// 次の Inspector に出す値に進む。enterChildren が false なら子を飛ばす。もうなければ false
	bool NextVisible(bool enterChildren);
	bool HasVisibleChildren() const;
	std::optional<SerializedProperty> FindPropertyRelative(std::string_view relativePath) const;

	bool IsArray() const;
	int GetArraySize() const;
	void SetArraySize(int size); // 増やすときは最後の要素を写す（Unity と同じ）
	std::optional<SerializedProperty> GetArrayElementAtIndex(int index) const;

	bool GetBoolValue() const;
	void SetBoolValue(bool value);
	int GetIntValue() const;
	void SetIntValue(int value);
	uint32_t GetUintValue() const;
	void SetUintValue(uint32_t value);
	float GetFloatValue() const;
	void SetFloatValue(float value);
	std::string GetStringValue() const;
	void SetStringValue(std::string_view value);
	Vector2 GetVector2Value() const;
	void SetVector2Value(const Vector2 &value);
	Vector3 GetVector3Value() const;
	void SetVector3Value(const Vector3 &value);
	Vector4 GetVector4Value() const;
	void SetVector4Value(const Vector4 &value);
	Quaternion GetQuaternionValue() const;
	void SetQuaternionValue(const Quaternion &value);
	Color GetColorValue() const;
	void SetColorValue(const Color &value);
	Rect GetRectValue() const;
	void SetRectValue(const Rect &value);

	Object *GetObjectReferenceValue() const;
	void SetObjectReferenceValue(Object *value); // 入れられない型・保存されないものなら何もしない

	std::span<const std::string_view> GetEnumNames() const;
	int GetEnumValueIndex() const; // 名前が見つからなければ -1
	void SetEnumValueIndex(int index);
	int GetEnumValueFlag() const;  // フラグのとき。ビット i が GetEnumNames()[i]
	void SetEnumValueFlag(int flags);


private: // メンバ変数

	SerializedObject *object_ = nullptr;
	std::string path_; // JSON Pointer（"/localPosition"）。GetIterator が返したばかりのときは空


private: // メンバ関数

	SerializedProperty(SerializedObject *object, std::string path);

	const PropertySchema *FindSchema() const;
	const Json *FindValue() const;
	void Write(Json value);

	template <size_t N>
	bool ReadFloats(float (&out)[N]) const;
	template <size_t N>
	void WriteFloats(const float (&values)[N]);

	bool Accepts(const Object *value) const;

	// Quaternion を度数で見せるときのヒント（EditorGUILayout が使う）
	std::optional<SerializedProperty> FindEulerHint() const;
};

} // namespace ou
