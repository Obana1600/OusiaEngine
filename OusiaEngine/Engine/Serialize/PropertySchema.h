#pragma once

#include "Serialize/Serializer.h"

#include <span>
#include <string>
#include <string_view>


namespace ou {

// Unity の SerializedPropertyType（使うものだけ）
enum class SerializedPropertyType {
	Generic,         // 中に値を持つもの（Struct・Array）
	Integer,
	Boolean,
	Float,
	String,
	Color,
	ObjectReference, // シーンの GameObject・Component か、アセット
	Enum,
	Vector2,
	Vector3,
	Vector4,
	Rect,
	Quaternion,
};

// JsonWriter が書いた値 1 つの型（Unity の type tree の 1 行にあたる）
// SerializedObject::Update が JsonWriter に書き留めさせ、Inspector の欄を作るのに使う
struct PropertySchema {
	std::string path;     // JSON Pointer（"/localPosition"・"/materials/0"）
	std::string name;     // フィールドの名前。配列の要素は "Element 0"
	SerializedPropertyType type = SerializedPropertyType::Generic;
	int depth = 0;        // 入れ子の深さ（一番上が 0）
	bool isVisible = true;  // false なら Inspector に出さない（Transform の localEulerAnglesHint など）
	bool isArray = false;   // Generic のうち配列
	bool isUnsigned = false; // Integer のうち uint32_t
	bool isFlags = false;    // Enum のうちフラグ（"A|B" と書く）
	bool isAsset = false;    // ObjectReference のうちアセット（guid で書く）

	std::span<const std::string_view> enumNames;  // Enum の名前（magic_enum の static なものを指す）
	Serializer::ObjectFilter accepts = nullptr;   // ObjectReference に入れてよい型か
	std::string_view objectTypeName;              // アセットの型の名前（"Material" など）
	std::string hintPath; // Quaternion を Inspector で度数で見せるときの、オイラー角のヒントのパス（RotationField）
};

} // namespace ou
