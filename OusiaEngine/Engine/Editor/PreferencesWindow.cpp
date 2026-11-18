#include "PreferencesWindow.h"
#include "Core/StringUtil.h"
#include "Editor/EditorGUILayout.h"
#include "Editor/ExternalScriptEditor.h"

#include <cfloat>
#include <chrono>
#include <future>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <Windows.h>
#include <commdlg.h>
#include <imgui/imgui.h>

// GetOpenFileNameW
#pragma comment(lib, "Comdlg32.lib")

using namespace ou;


namespace {

// Unity と同じく、一覧にないエディタは exe を選んで足せる
std::optional<std::filesystem::path> BrowseExecutable() {
	wchar_t buffer[MAX_PATH] = {};
	OPENFILENAMEW dialog{};
	dialog.lStructSize = sizeof(dialog);
	dialog.lpstrFilter = L"実行ファイル (*.exe)\0*.exe\0";
	dialog.lpstrFile = buffer;
	dialog.nMaxFile = MAX_PATH;
	dialog.lpstrTitle = L"スクリプトを開くエディタ";
	dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	if (!GetOpenFileNameW(&dialog)) { return std::nullopt; }
	return std::filesystem::path(buffer);
}

} // namespace



ou::PreferencesWindow::PreferencesWindow() {
	SetTitle("Preferences");
}


void ou::PreferencesWindow::OnGui() {
	DrawExternalTools();
}


void ou::PreferencesWindow::DrawExternalTools() {
	ImGui::SeparatorText("External Tools");

	// Visual Studio を探す（vswhere）のに数秒かかるので、別のスレッドで探し、見つかるまでは「探しています」と出す
	static auto search = std::async(std::launch::async, &ExternalScriptEditor::FindInstallations);
	static std::vector<ExternalScriptEditor::Installation> installations;
	static auto isSearching = true;
	if (isSearching && search.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
		installations = search.get();
		isSearching = false;
	}
	const auto current = ExternalScriptEditor::FindCurrentPath();

	std::string currentName = current.empty() ? "（見つかりません）" : ToUtf8(current.filename());
	for (const auto &installation : installations) {
		if (installation.path == current) { currentName = installation.name; }
	}

	EditorGuiLayout::PrefixLabel("External Script Editor");
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::BeginCombo("##ExternalScriptEditor", currentName.c_str())) {
		for (const auto &installation : installations) {
			if (ImGui::Selectable(installation.name.c_str(), installation.path == current)) {
				ExternalScriptEditor::SetCurrentPath(installation.path);
			}
			ImGui::SetItemTooltip("%s", ToUtf8(installation.path).c_str());
		}
		if (isSearching) { ImGui::TextDisabled("エディタを探しています..."); }
		ImGui::Separator();
		if (ImGui::Selectable("Browse...")) {
			if (const auto path = BrowseExecutable()) { ExternalScriptEditor::SetCurrentPath(*path); }
		}
		ImGui::EndCombo();
	}

	ImGui::TextDisabled("%s", current.empty() ? "" : ToUtf8(current).c_str());
	ImGui::TextDisabled("Visual Studio Code ではプロジェクトのフォルダを開くので、AngelScript Language Server の補完が効きます");
}
