#pragma once

#include "Diagnostics/Log.h"
#include "Serialize/Serializer.h"

#include <string>
#include <string_view>
#include <type_traits>

#include <magic_enum/magic_enum_flags.hpp>


namespace ou {

template <typename E>
void SerializeEnums(Serializer &serializer, std::string_view name, E &value) {
	static_assert(std::is_enum_v<E>, "E は enum でなければいけません");

	auto text = std::string(magic_enum::enum_flags_name(value));

	if (!serializer.IsReading()) {
		const auto isZero = static_cast<std::underlying_type_t<E>>(value) == 0;
		if (!isZero && text.empty()) {
			Log::Warning("フラグに名前のないビットがあるため保存できません: \"{}\"", name);
			return;
		}
		serializer.Field(name, text);
		return;
	}

	const auto before = text;
	serializer.Field(name, text);
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

} // namespace ou
