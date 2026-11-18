#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <utility>


namespace ou {

class Texture;

// Unity の GuiContent。欄に出す文字・文字の左に出す小さな画像・マウスを乗せたときのツールチップ
struct GuiContent {
	static const GuiContent none;

	std::string text;
	std::shared_ptr<Texture> image;
	std::string tooltip;

	GuiContent() = default;
	GuiContent(std::string_view text)
		: text(text) {}
	GuiContent(std::shared_ptr<Texture> image)
		: image(std::move(image)) {}
	GuiContent(std::string_view text, std::shared_ptr<Texture> image)
		: text(text),
		  image(std::move(image)) {}
	GuiContent(std::string_view text, std::string_view tooltip)
		: text(text),
		  tooltip(tooltip) {}
	GuiContent(std::shared_ptr<Texture> image, std::string_view tooltip)
		: image(std::move(image)),
		  tooltip(tooltip) {}
	GuiContent(std::string_view text, std::shared_ptr<Texture> image, std::string_view tooltip)
		: text(text),
		  image(std::move(image)),
		  tooltip(tooltip) {}
};

inline const GuiContent GuiContent::none;

} // namespace ou
