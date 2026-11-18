#include "BuildSettingsWindow.h"
#include "Asset/AssetDatabase.h"
#include "Diagnostics/Log.h"
#include "Editor/ProjectSettingsWindow.h"
#include "Core/StringUtil.h"
#include "Editor/EditorPayload.h"
#include "Editor/BuildPipeline.h"
#include "Editor/EditorGuiLayout.h"
#include "Editor/EditorPrefs.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"

#include <algorithm>
#include <cfloat>
#include <filesystem>
#include <thread>
#include <cctype>
#include <optional>
#include <string>
#include <utility>

#include <Windows.h>
#include <shobjidl.h>
#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

using namespace ou;


namespace {

// 一覧の中で並べ替えるときの payload
constexpr const char *kRowPayload = "OU_BUILD_SCENE_ROW";

bool IsScenePath(std::string_view path) {
	constexpr std::string_view kExtension = ".scene";
	if (path.size() < kExtension.size()) { return false; }

	const auto tail = path.substr(path.size() - kExtension.size());
	return std::equal(tail.begin(), tail.end(), kExtension.begin(), [](char a, char b) {
		return std::tolower(static_cast<unsigned char>(a)) == b;
	});
}


// Unity と同じく、書き出す場所は前回の場所から選ぶ（PC ごと）
constexpr const char *kBuildLocationKey = "BuildLocation";

// フォルダを選ぶダイアログ。IFileOpenDialog は STA で使うものなので、別のスレッドで開いて待つ（エンジンは MTA で COM を初期化している）
std::optional<std::filesystem::path> PickFolder(const std::filesystem::path &initialFolder) {
	std::optional<std::filesystem::path> result;
	std::thread([&] {
		if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) { return; }

		IFileOpenDialog *dialog = nullptr;
		if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
			DWORD options = 0;
			dialog->GetOptions(&options);
			dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
			dialog->SetTitle(L"書き出す場所");

			IShellItem *folder = nullptr;
			if (!initialFolder.empty() && SUCCEEDED(SHCreateItemFromParsingName(initialFolder.c_str(), nullptr, IID_PPV_ARGS(&folder)))) {
				dialog->SetFolder(folder);
				folder->Release();
			}

			IShellItem *item = nullptr;
			if (SUCCEEDED(dialog->Show(nullptr)) && SUCCEEDED(dialog->GetResult(&item))) {
				PWSTR path = nullptr;
				if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
					result = std::filesystem::path(path);
					CoTaskMemFree(path);
				}
				item->Release();
			}
			dialog->Release();
		}
		CoUninitialize();
	}).join();
	return result;
}

} // namespace



ou::BuildSettingsWindow::BuildSettingsWindow() {
	SetTitle("Build Settings");
}


void ou::BuildSettingsWindow::OnGui() {
	// 開いたときに読む（ファイルを手で直したときは、閉じて開き直す）
	if (!isLoaded_) {
		scenes_ = EditorBuildSettings::GetScenes();
		isLoaded_ = true;
	}

	ImGui::TextUnformatted("Scenes In Build");
	DrawSceneList();

	// Project から .scene を落とすと足す
	if (ImGui::BeginDragDropTarget()) {
		if (const auto *payload = ImGui::AcceptDragDropPayload(EditorPayload::kAsset)) {
			const auto asset = EditorPayload::DecodeAsset(payload->Data, payload->DataSize);
			if (asset.subAsset.empty() && IsScenePath(asset.path)) {
				AddScene(asset.path);
			}
		}
		ImGui::EndDragDropTarget();
	}

	if (ImGui::Button("Add Open Scenes")) {
		const auto *scene = SceneManager::GetActiveScene();
		if (scene && !scene->GetPath().empty()) {
			AddScene(scene->GetPath());
		} else {
			Log::Warning("保存していないシーンは足せません。先にシーンを保存してください");
		}
	}
	ImGui::SameLine();
	ImGui::TextDisabled("Project から .scene を引いて足せます。右クリックか Delete で外します");

	DrawBuildArea();
}


void ou::BuildSettingsWindow::DrawSceneList() {
	const auto height = std::max(ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() * 6.0f, ImGui::GetFrameHeight() * 3.0f);
	if (!ImGui::BeginChild("##Scenes", {0.0f, height}, ImGuiChildFlags_Borders)) {
		ImGui::EndChild();
		return;
	}

	auto changed = false;
	std::optional<std::pair<int, int>> move; // (from, to)
	std::optional<int> remove;

	auto buildIndex = 0;
	for (int i = 0; i < static_cast<int>(scenes_.size()); i++) {
		auto &scene = scenes_[i];
		ImGui::PushID(i);

		const auto rowRight = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;

		if (ImGui::Checkbox("##Enabled", &scene.enabled)) { changed = true; }
		ImGui::SameLine();

		if (ImGui::Selectable(scene.path.c_str(), selected_ == i, ImGuiSelectableFlags_AllowOverlap)) {
			selected_ = i;
		}

		// 引いて並べ替える
		if (ImGui::BeginDragDropSource()) {
			ImGui::SetDragDropPayload(kRowPayload, &i, sizeof(i));
			ImGui::TextUnformatted(scene.path.c_str());
			ImGui::EndDragDropSource();
		}
		if (ImGui::BeginDragDropTarget()) {
			if (const auto *payload = ImGui::AcceptDragDropPayload(kRowPayload)) {
				move = std::pair{*static_cast<const int *>(payload->Data), i};
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::MenuItem("Remove Selection")) { remove = i; }
			ImGui::EndPopup();
		}

		// 有効なシーンだけに build index を振る（Unity と同じく右端に出す）
		if (scene.enabled) {
			const auto text = std::to_string(buildIndex++);
			ImGui::SameLine(rowRight - ImGui::CalcTextSize(text.c_str()).x);
			ImGui::TextDisabled("%s", text.c_str());
		}

		ImGui::PopID();
	}

	if (scenes_.empty()) {
		ImGui::TextDisabled("シーンがありません。Player は始められません");
	}

	if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete) && selected_ >= 0) {
		remove = selected_;
	}

	ImGui::EndChild();

	if (move && move->first != move->second) {
		auto moving = std::move(scenes_[move->first]);
		scenes_.erase(scenes_.begin() + move->first);
		scenes_.insert(scenes_.begin() + move->second, std::move(moving));
		selected_ = move->second;
		changed = true;
	}
	if (remove && *remove < static_cast<int>(scenes_.size())) {
		scenes_.erase(scenes_.begin() + *remove);
		selected_ = -1;
		changed = true;
	}

	if (changed) { Save(); }
}


void ou::BuildSettingsWindow::AddScene(const std::string &scenePath) {
	const auto exists = std::any_of(scenes_.begin(), scenes_.end(), [&](const auto &scene) {
		return scene.path == scenePath;
	});
	if (exists) { return; }

	scenes_.push_back({scenePath, AssetDatabase::AssetPathToGuid(scenePath), true});
	Save();
}


void ou::BuildSettingsWindow::Save() {
	EditorBuildSettings::SetScenes(scenes_);
}


void ou::BuildSettingsWindow::DrawBuildArea() {
	ImGui::Separator();

	// Unity と同じく、Player の設定は Project Settings で変える
	if (ImGui::Button("Player Settings...")) { ProjectSettingsWindow::Open(ProjectSettingsWindow::Section::Player); }

	if (ImGui::Button("Build...", {-FLT_MIN, 0.0f})) {
		const auto previous = std::filesystem::path(Utf8ToWide(EditorPrefs::GetString(kBuildLocationKey)));
		if (const auto location = PickFolder(previous)) {
			EditorPrefs::SetString(kBuildLocationKey, ToUtf8(*location));
			BuildPipeline::BuildPlayer(*location);
		}
	}
}
