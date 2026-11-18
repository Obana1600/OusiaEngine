#include "EditorWindow.h"

#include <imgui/imgui.h>

using namespace ou;


void ou::EditorWindow::Show() {
	isOpen_ = true;
}


void ou::EditorWindow::Close() {
	isOpen_ = false;
}


void ou::EditorWindow::Focus() {
	wantsFocus_ = true;
}


void ou::EditorWindow::SetTitle(std::string_view title) {
	title_ = title;
}


const std::string &ou::EditorWindow::GetTitle() const {
	return title_;
}


void ou::EditorWindow::DrawAll() {
	for (const auto &window : sWindows_) {
		if (!window->isOpen_) { continue; }

		if (std::exchange(window->wantsFocus_, false)) {
			ImGui::SetNextWindowFocus();
		}

		if (ImGui::Begin(window->title_.c_str(), &window->isOpen_, ImGuiWindowFlags_NoCollapse)) {
			window->OnGui();
		}
		ImGui::End();
	}
}


void ou::EditorWindow::DestroyAll() {
	sWindows_.clear();
}
