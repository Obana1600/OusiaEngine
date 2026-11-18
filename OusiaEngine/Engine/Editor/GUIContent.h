#pragma once

#include <string>
#include <string_view>


namespace ou {

// Unity の GUIContent。欄に出す文字と、マウスを乗せたときのツールチップ
struct GUIContent {
	std::string text;
	std::string tooltip;

	GUIContent() = default;
	GUIContent(std::string_view text, std::string_view tooltip = {})
		: text(text),
		  tooltip(tooltip) {}
};

} // namespace ou
