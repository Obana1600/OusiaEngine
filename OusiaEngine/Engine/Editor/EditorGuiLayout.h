#pragma once

#include <string_view>


namespace ou {

class EditorGUILayout final {
public: // 静的メンバ関数

	EditorGUILayout() = delete;

	static void PrefixLabel(std::string_view label);
};

} // namespace ou
