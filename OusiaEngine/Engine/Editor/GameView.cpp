#include "GameView.h"
#include "Editor/EditorFunc.h"
#include "Editor/ImGuiManager.h"

#include <imgui/imgui.h>

using namespace ou;


namespace {

GameView *sLastDrawn = nullptr;
int sLastDrawnFrame = -1;

} // namespace



GameView *ou::GameView::FindVisible() {
	if (sLastDrawnFrame != ImGui::GetFrameCount()) { return nullptr; }
	return sLastDrawn;
}


ou::GameView::GameView() {
	SetTitle("Game");
}


const SceneRenderTarget &ou::GameView::GetRenderTarget() const {
	return renderTarget_;
}


const Vector2 &ou::GameView::GetImageOrigin() const {
	return imageOrigin_;
}


bool ou::GameView::HasFocus() const {
	return hasFocus_;
}


bool ou::GameView::IsImageHovered() const {
	return isImageHovered_;
}


void ou::GameView::OnGui() {
	const auto size = ImGui::GetContentRegionAvail();
	if (size.x < 1.0f || size.y < 1.0f) { return; }

	renderTarget_.Resize(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
	const auto origin = ImGui::GetCursorScreenPos();
	imageOrigin_ = {origin.x, origin.y};
	hasFocus_ = ImGui::IsWindowFocused();
	ImGui::Image(::ToTextureId(renderTarget_.GetColor()), size);
	isImageHovered_ = ImGui::IsItemHovered();

	sLastDrawn = this;
	sLastDrawnFrame = ImGui::GetFrameCount();
}
