#include "EditorGuiUtility.h"

#include <utility>

using namespace ou;


GuiContent ou::EditorGuiUtility::TrTextContent(std::string_view text, std::string_view tooltip, std::shared_ptr<Texture> icon) {
	return GuiContent(text, std::move(icon), tooltip);
}


GuiContent ou::EditorGuiUtility::TrTextContentWithIcon(std::string_view text, std::shared_ptr<Texture> icon) {
	return GuiContent(text, std::move(icon));
}


GuiContent ou::EditorGuiUtility::TrTextContentWithIcon(std::string_view text, std::string_view tooltip, std::shared_ptr<Texture> icon) {
	return GuiContent(text, std::move(icon), tooltip);
}


GuiContent ou::EditorGuiUtility::TrIconContent(std::shared_ptr<Texture> icon, std::string_view tooltip) {
	return GuiContent(std::move(icon), tooltip);
}
