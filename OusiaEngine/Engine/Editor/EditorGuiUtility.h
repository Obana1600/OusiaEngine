#pragma once

#include "Editor/GuiContent.h"

#include <memory>
#include <string_view>


namespace ou {

// Unity の EditorGuiUtility。エディタの欄に出す GuiContent を作る
// Tr は Unity では訳して作るという意味。訳す仕組みはまだないので、渡した文字のまま作る
class EditorGuiUtility final {
public: // 静的メンバ関数

	EditorGuiUtility() = delete;

	static GuiContent TrTextContent(std::string_view text, std::string_view tooltip = {}, std::shared_ptr<Texture> icon = nullptr);
	static GuiContent TrTextContentWithIcon(std::string_view text, std::shared_ptr<Texture> icon);
	static GuiContent TrTextContentWithIcon(std::string_view text, std::string_view tooltip, std::shared_ptr<Texture> icon);
	static GuiContent TrIconContent(std::shared_ptr<Texture> icon, std::string_view tooltip = {});
};

} // namespace ou
