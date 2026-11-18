#include "ProjectWindow.h"
#include "Asset/AssetDatabase.h"
#include "Asset/AssetImporter.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorMainWindow.h"
#include "Editor/EditorPayload.h"
#include "Editor/ExternalScriptEditor.h"
#include "Platform/Paths.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <fstream>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <Windows.h>
#include <shellapi.h>
#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

using namespace ou;


namespace {

constexpr const char *kCreatePopup = "Create";

struct Root {
	std::string name;
	std::filesystem::path fullPath;
};

std::vector<Root> FindRoots() {
	std::vector<Root> roots;
	if (auto assets = FindAssetsFolder()) { roots.push_back({"Assets", *assets}); }
	if (auto engineAssets = FindEngineAssetsFolder()) { roots.push_back({"EngineAssets", *engineAssets}); }
	return roots;
}

// アセットのパスを実際のフォルダにする
std::optional<std::filesystem::path> ToFullPath(const std::string &assetPath) {
	const auto slash = assetPath.find('/');
	const auto rootName = assetPath.substr(0, slash);
	for (const auto &root : FindRoots()) {
		if (root.name != rootName) { continue; }
		if (slash == std::string::npos) { return root.fullPath; }
		return root.fullPath / Utf8ToWide(assetPath.substr(slash + 1));
	}
	return std::nullopt;
}

std::string LowerExtension(const std::filesystem::path &path) {
	auto extension = ToUtf8(path.extension());
	std::transform(extension.begin(), extension.end(), extension.begin(),
				   [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
	return extension;
}

bool IsVisible(const std::filesystem::path &path) {
	return !ToUtf8(path.filename()).starts_with('.') && LowerExtension(path) != ".meta";
}

bool IsModelFile(const std::filesystem::path &path) {
	const auto extension = LowerExtension(path);
	return extension == ".gltf" || extension == ".glb" || extension == ".obj";
}

bool IsSceneFile(const std::string &assetPath) {
	return LowerExtension(std::filesystem::path(Utf8ToWide(assetPath))) == ".scene";
}

bool IsScriptFile(const std::string &assetPath) {
	return LowerExtension(std::filesystem::path(Utf8ToWide(assetPath))) == ".as";
}

// フォルダ → ファイルの順に、名前で並べる
std::vector<std::filesystem::directory_entry> ListEntries(const std::filesystem::path &folder, bool foldersOnly) {
	std::vector<std::filesystem::directory_entry> entries;
	std::error_code error;
	for (const auto &entry : std::filesystem::directory_iterator(folder, error)) {
		if (!IsVisible(entry.path())) { continue; }
		if (foldersOnly && !entry.is_directory()) { continue; }
		entries.push_back(entry);
	}
	std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
		if (a.is_directory() != b.is_directory()) { return a.is_directory(); }
		return a.path().filename() < b.path().filename();
	});
	return entries;
}

// ファイル名＝クラス名なので、AngelScript のクラス名に使える名前だけにする
bool IsValidClassName(std::string_view name) {
	if (name.empty() || std::isdigit(static_cast<unsigned char>(name.front()))) { return false; }
	return std::all_of(name.begin(), name.end(), [](char c) {
		return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
	});
}

bool IsValidFolderName(std::string_view name) {
	return !name.empty() && name.find_first_of("\\/:*?\"<>|") == std::string_view::npos && name != "." && name != "..";
}

// Unity の NewBehaviourScript.cs と同じく、Start と Update だけを書いておく
std::string MakeScriptTemplate(std::string_view className) {
	return std::format(
		"class {} : MonoBehavior {{\r\n"
		"\t// 最初の Update の前に 1 回だけ呼ばれる\r\n"
		"\tvoid Start() {{\r\n"
		"\t}}\r\n"
		"\r\n"
		"\t// 毎フレーム呼ばれる\r\n"
		"\tvoid Update() {{\r\n"
		"\t}}\r\n"
		"}}\r\n",
		className);
}

// エクスプローラーで、そのファイルを選んだ状態で開く
void ShowInExplorer(const std::filesystem::path &path) {
	const auto parameters = std::format(L"/select,\"{}\"", path.wstring());
	ShellExecuteW(nullptr, L"open", L"explorer.exe", parameters.c_str(), nullptr, SW_SHOWNORMAL);
}

} // namespace



ou::ProjectWindow::ProjectWindow() {
	SetTitle("Project");
}


void ou::ProjectWindow::OnGui() {
	constexpr auto kTableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;
	if (ImGui::BeginTable("##Project", 2, kTableFlags, ImGui::GetContentRegionAvail())) {
		ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 200.0f);
		ImGui::TableSetupColumn("Contents", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		if (ImGui::BeginChild("##Folders")) {
			for (const auto &root : FindRoots()) {
				DrawFolderTree(root.name, root.fullPath);
			}
			revealSelectedFolder_ = false;
		}
		ImGui::EndChild();

		ImGui::TableSetColumnIndex(1);
		if (ImGui::BeginChild("##Contents")) {
			DrawContents();
		}
		ImGui::EndChild();

		ImGui::EndTable();
	}

	DrawCreatePopup();
}


void ou::ProjectWindow::DrawFolderTree(const std::string &assetPath, const std::filesystem::path &fullPath) {
	const auto subFolders = ListEntries(fullPath, true);

	auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (assetPath == selectedFolder_) { flags |= ImGuiTreeNodeFlags_Selected; }
	if (subFolders.empty()) { flags |= ImGuiTreeNodeFlags_Leaf; }

	if (revealSelectedFolder_ && selectedFolder_.starts_with(assetPath + "/")) {
		ImGui::SetNextItemOpen(true);
	}

	const auto isOpen = ImGui::TreeNodeEx(assetPath.c_str(), flags, "%s", ToUtf8(fullPath.filename()).c_str());
	if (revealSelectedFolder_ && assetPath == selectedFolder_) {
		ImGui::SetScrollHereY();
	}
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
		selectedFolder_ = assetPath;
	}

	if (!isOpen) { return; }
	for (const auto &entry : subFolders) {
		DrawFolderTree(assetPath + "/" + ToUtf8(entry.path().filename()), entry.path());
	}
	ImGui::TreePop();
}


void ou::ProjectWindow::DrawContents() {
	ImGui::TextDisabled("%s", selectedFolder_.c_str());
	ImGui::Separator();

	const auto folder = ToFullPath(selectedFolder_);
	std::error_code error;
	if (!folder || !std::filesystem::is_directory(*folder, error)) {
		ImGui::TextDisabled("フォルダが見つかりません");
		return;
	}

	const auto currentFolder = selectedFolder_;
	for (const auto &entry : ListEntries(*folder, false)) {
		const auto name = ToUtf8(entry.path().filename());
		const auto assetPath = currentFolder + "/" + name;
		if (entry.is_directory()) {
			DrawFolderItem(assetPath, name);
		} else {
			DrawFileItem(assetPath, entry.path());
		}
	}

	// 何もないところの右クリック（Unity の Project の Create）
	if (ImGui::BeginPopupContextWindow("##ContentsMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
		DrawCreateMenu();
		ImGui::Separator();
		if (ImGui::MenuItem("Show in Explorer")) { ShowInExplorer(*folder); }
		ImGui::EndPopup();
	}
}


void ou::ProjectWindow::DrawFolderItem(const std::string &assetPath, const std::string &name) {
	constexpr auto kFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	const auto isSelected = selectedItem_ == EditorPayload::EncodeAsset(assetPath, {});
	ImGui::TreeNodeEx(assetPath.c_str(), kFlags | (isSelected ? ImGuiTreeNodeFlags_Selected : 0), "%s/", name.c_str());

	if (ImGui::IsItemClicked()) {
		selectedItem_ = EditorPayload::EncodeAsset(assetPath, {});
	}
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		selectedFolder_ = assetPath;
		revealSelectedFolder_ = true;
	}
	DrawItemMenu(assetPath);
}


void ou::ProjectWindow::DrawFileItem(const std::string &assetPath, const std::filesystem::path &fullPath) {
	const auto isModel = IsModelFile(fullPath);

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
	flags |= isModel ? ImGuiTreeNodeFlags_OpenOnArrow : (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
	if (selectedItem_ == EditorPayload::EncodeAsset(assetPath, {})) { flags |= ImGuiTreeNodeFlags_Selected; }

	const auto isOpen = ImGui::TreeNodeEx(assetPath.c_str(), flags, "%s", ToUtf8(fullPath.filename()).c_str());
	HandleItem(assetPath, {});
	DrawItemMenu(assetPath);

	if (isModel && isOpen) {
		DrawSubAssets(assetPath);
		ImGui::TreePop();
	}
}


void ou::ProjectWindow::DrawSubAssets(const std::string &assetPath) {
	if (!AssetDatabase::LoadMainAssetAtPath(assetPath)) { return; }
	const auto *importer = AssetDatabase::GetImporterAtPath(assetPath);
	if (!importer) { return; }

	constexpr auto kFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	for (const auto &sub : importer->GetSubAssets()) {
		const auto key = EditorPayload::EncodeAsset(assetPath, sub.name);
		ImGui::TreeNodeEx(key.c_str(), kFlags | (selectedItem_ == key ? ImGuiTreeNodeFlags_Selected : 0), "%s", sub.name.c_str());
		HandleItem(assetPath, sub.name);
	}
}


void ou::ProjectWindow::HandleItem(const std::string &assetPath, const std::string &subAsset) {
	const auto payload = EditorPayload::EncodeAsset(assetPath, subAsset);

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
		selectedItem_ = payload;
	}

	// ダブルクリックで開く（.scene はシーン、.as は外部のエディタ）
	if (subAsset.empty() && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		OpenAsset(assetPath);
	}

	if (ImGui::BeginDragDropSource()) {
		ImGui::SetDragDropPayload(EditorPayload::kAsset, payload.c_str(), payload.size() + 1);
		ImGui::Text("%s%s%s", assetPath.c_str(), subAsset.empty() ? "" : " : ", subAsset.c_str());
		ImGui::EndDragDropSource();
	}
}


void ou::ProjectWindow::DrawItemMenu(const std::string &assetPath) {
	if (!ImGui::BeginPopupContextItem("##ItemMenu")) { return; }

	selectedItem_ = EditorPayload::EncodeAsset(assetPath, {});
	DrawCreateMenu();
	ImGui::Separator();

	const auto canOpen = IsScriptFile(assetPath) || IsSceneFile(assetPath);
	if (ImGui::MenuItem("Open", nullptr, false, canOpen)) { OpenAsset(assetPath); }
	if (ImGui::MenuItem("Show in Explorer")) {
		if (const auto fullPath = ToFullPath(assetPath)) { ShowInExplorer(*fullPath); }
	}
	ImGui::EndPopup();
}


void ou::ProjectWindow::DrawCreateMenu() {
	if (!ImGui::BeginMenu("Create")) { return; }
	if (ImGui::MenuItem("Folder")) { BeginCreate(CreateKind::Folder); }
	ImGui::Separator();
	if (ImGui::MenuItem("MonoBehavior Script")) { BeginCreate(CreateKind::Script); }
	ImGui::EndMenu();
}


void ou::ProjectWindow::BeginCreate(CreateKind kind) {
	// EngineAssets はエンジンのものなので、作るのは Assets の中だけ
	creatingFolder_ = selectedFolder_.starts_with("Assets") ? selectedFolder_ : "Assets";
	creatingKind_ = kind;
	creatingName_ = kind == CreateKind::Folder ? "New Folder" : "NewBehavior";
	opensCreatePopup_ = true;
}


void ou::ProjectWindow::DrawCreatePopup() {
	if (std::exchange(opensCreatePopup_, false)) {
		ImGui::OpenPopup(kCreatePopup);
	}
	if (!ImGui::BeginPopupModal(kCreatePopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) { return; }

	const auto isScript = creatingKind_ == CreateKind::Script;
	ImGui::Text("%s に%s", creatingFolder_.c_str(), isScript ? "スクリプトを作ります（ファイル名がクラス名になります）" : "フォルダを作ります");

	if (ImGui::IsWindowAppearing()) {
		ImGui::SetKeyboardFocusHere();
	}
	ImGui::SetNextItemWidth(360.0f);
	const auto isEntered = ImGui::InputText("##Name", &creatingName_, ImGuiInputTextFlags_EnterReturnsTrue);

	const auto folder = ToFullPath(creatingFolder_);
	const auto fileName = isScript ? creatingName_ + ".as" : creatingName_;
	const auto fullPath = folder ? *folder / Utf8ToWide(fileName) : std::filesystem::path();

	std::error_code error;
	std::string problem;
	if (isScript && !IsValidClassName(creatingName_)) {
		problem = "英字か _ で始め、英数字と _ だけにしてください（クラス名になります）";
	} else if (!isScript && !IsValidFolderName(creatingName_)) {
		problem = "フォルダ名に使えない文字があります";
	} else if (!folder || std::filesystem::exists(fullPath, error)) {
		problem = "同じ名前のものがもうあります";
	}
	if (!problem.empty()) {
		ImGui::TextColored({1.0f, 0.4f, 0.4f, 1.0f}, "%s", problem.c_str());
	}

	ImGui::BeginDisabled(!problem.empty());
	const auto isCreateClicked = ImGui::Button("作成");
	ImGui::EndDisabled();

	if ((isCreateClicked || isEntered) && problem.empty()) {
		auto isCreated = false;
		if (isScript) {
			std::ofstream file(fullPath, std::ios::binary);
			file << MakeScriptTemplate(creatingName_);
			isCreated = static_cast<bool>(file);
		} else {
			isCreated = std::filesystem::create_directory(fullPath, error);
		}

		if (isCreated) {
			// .meta を作る。スクリプトのコンパイルは、書き換えを見ているエディタが 1 秒以内にする
			AssetDatabase::Refresh();
			selectedItem_ = EditorPayload::EncodeAsset(creatingFolder_ + "/" + fileName, {});
			Log::Info("{}を作りました: {}/{}", isScript ? "スクリプト" : "フォルダ", creatingFolder_, fileName);
		} else {
			Log::Error("作れませんでした: {}", ToUtf8(fullPath));
		}
		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine();
	if (ImGui::Button("キャンセル")) {
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();
}


void ou::ProjectWindow::OpenAsset(const std::string &assetPath) {
	if (IsSceneFile(assetPath)) {
		EditorMainWindow::RequestOpenScene(assetPath);
		return;
	}
	if (IsScriptFile(assetPath)) {
		if (const auto fullPath = ToFullPath(assetPath)) { ExternalScriptEditor::OpenFile(*fullPath); }
	}
}
