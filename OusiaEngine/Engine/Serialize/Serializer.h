#pragma once

#include "Core/Object.h"
#include "Diagnostics/Log.h"
#include "Math/Color.h"
#include "Math/Quaternion.h"
#include "Math/Rect.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_flags.hpp>
#include <span>


namespace ou {

class Serializer {
public: // エイリアス

	using ObjectFilter = bool (*)(const Object *);


public: // メンバ関数

	virtual ~Serializer() = default;

	virtual bool IsReading() const = 0;

	virtual void Field(std::string_view name, bool &value) = 0;
	virtual void Field(std::string_view name, int32_t &value) = 0;
	virtual void Field(std::string_view name, uint32_t &value) = 0;
	virtual void Field(std::string_view name, float &value) = 0;
	virtual void Field(std::string_view name, Vector2 &value) = 0;
	virtual void Field(std::string_view name, Vector3 &value) = 0;
	virtual void Field(std::string_view name, Vector4 &value) = 0;
	virtual void Field(std::string_view name, Quaternion &value) = 0;
	virtual void Field(std::string_view name, Color &value) = 0;
	virtual void Field(std::string_view name, Rect &value) = 0;
	virtual void Field(std::string_view name, std::string &value) = 0;

	virtual void RotationField(std::string_view name, Quaternion &rotation, std::string_view hintName, Vector3 &eulerHint);

	virtual bool BeginObject(std::string_view name) = 0;
	virtual void EndObject() = 0;

	virtual bool BeginArray(std::string_view name, size_t &count) = 0;
	virtual void EndArray() = 0;

	template <typename T>
	void Reference(std::string_view name, T *&value);

	template <typename T>
	void Asset(std::string_view name, std::shared_ptr<T> &value);

	template <typename E>
	void Enum(std::string_view name, E &value);

	template <typename E>
	void Flags(std::string_view name, E &value);

	template <typename T, typename F>
	void Array(std::string_view name, std::vector<T> &values, F &&element);

	template <typename F>
	void Struct(std::string_view name, F &&body);


protected: // インナークラス

	struct AssetType {
		std::string_view name;
		ObjectFilter accepts;
	};


protected: // メンバ関数

	virtual void ObjectReference(std::string_view name, Object *&value, ObjectFilter accepts) = 0;
	virtual void AssetObject(std::string_view name, std::shared_ptr<Object> &value, const AssetType &type) = 0;

	virtual void EnumField(std::string_view name, std::string &text, std::span<const std::string_view> names);
	virtual void FlagsField(std::string_view name, std::string &text, std::span<const std::string_view> names);


protected: // 静的メンバ関数

	template <typename T>
	static std::string_view MakeTypeName();
};



template <typename T>
inline void Serializer::Reference(std::string_view name, T *&value) {
	static_assert(std::is_base_of_v<Object, T>, "T は Object の派生クラスでなければいけません");

	Object *object = value;
	ObjectReference(name, object, [](const Object *candidate) {
		return dynamic_cast<const T *>(candidate) != nullptr;
	});

	if (IsReading()) {
		value = dynamic_cast<T *>(object);
		if (object && !value) { Log::Warning("参照先の型が違います: \"{}\"", name); }
	}
}


template <typename T>
inline void Serializer::Asset(std::string_view name, std::shared_ptr<T> &value) {
	static_assert(std::is_base_of_v<Object, T>, "T は Object の派生クラスでなければいけません");

	const AssetType type{
		MakeTypeName<T>(),
		[](const Object *candidate) { return dynamic_cast<const T *>(candidate) != nullptr; }};

	std::shared_ptr<Object> object = value;
	AssetObject(name, object, type);

	if (IsReading()) {
		value = std::dynamic_pointer_cast<T>(object);
		if (object && !value) { Log::Warning("アセットの型が違います: \"{}\"", name); }
	}
}


template <typename E>
inline void Serializer::Enum(std::string_view name, E &value) {
	static_assert(std::is_enum_v<E>, "E は enum でなければいけません");

	constexpr auto &kNames = magic_enum::enum_names<E>();
	auto text = std::string(magic_enum::enum_name(value));

	if (!IsReading()) {
		if (text.empty()) {
			Log::Warning("enum の値に名前がないため保存できません: \"{}\"", name);
			return;
		}
		EnumField(name, text, kNames);
		return;
	}

	const auto before = text;
	EnumField(name, text, kNames);
	if (const auto parsed = magic_enum::enum_cast<E>(text)) {
		value = *parsed;
		return;
	}

	std::string names;
	for (const auto candidate : kNames) {
		if (!names.empty()) { names += ", "; }
		names += candidate;
	}
	Log::Warning("enum の名前が違います: \"{}\": \"{}\" (使えるもの: {})", name, text, names);
}


template <typename E>
inline void Serializer::Flags(std::string_view name, E &value) {
	static_assert(std::is_enum_v<E>, "E は enum でなければいけません");

	constexpr auto &kNames = magic_enum::enum_names<E, magic_enum::as_flags<>>();
	auto text = std::string(magic_enum::enum_flags_name(value));

	if (!IsReading()) {
		const auto isZero = static_cast<std::underlying_type_t<E>>(value) == 0;
		if (!isZero && text.empty()) {
			Log::Warning("フラグに名前のないビットがあるため保存できません: \"{}\"", name);
			return;
		}
		FlagsField(name, text, kNames);
		return;
	}

	const auto before = text;
	FlagsField(name, text, kNames);
	if (text == before) { return; }

	if (text.empty()) {
		value = E{};
		return;
	}

	if (const auto parsed = magic_enum::enum_flags_cast<E>(text)) {
		value = *parsed;
		return;
	}
	Log::Warning("フラグの名前が違います: \"{}\": \"{}\"", name, text);
}


template <typename T, typename F>
inline void Serializer::Array(std::string_view name, std::vector<T> &values, F &&element) {
	auto count = values.size();
	if (!BeginArray(name, count)) { return; }

	if (IsReading()) {
		if (count > values.size() && !values.empty()) {
			const auto last = values.back();
			values.resize(count, last);
		} else {
			values.resize(count);
		}
	}

	for (auto &value : values) {
		element(value);
	}
	EndArray();
}


template <typename F>
inline void Serializer::Struct(std::string_view name, F &&body) {
	if (!BeginObject(name)) { return; }

	body();
	EndObject();
}


template <typename T>
inline std::string_view Serializer::MakeTypeName() {
	static const std::string name = [] {
		const std::string_view full = typeid(T).name();
		const auto last = full.find_last_of(": ");
		return std::string(last == std::string_view::npos ? full : full.substr(last + 1));
	}();
	return name;
}

} // namespace ou
