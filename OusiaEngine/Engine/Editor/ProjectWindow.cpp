#include "ProjectWindow.h"
#include "Asset/AssetDatabase.h"
#include "Asset/AssetImporter.h"
#include "Core/StringUtil.h"
#include "Editor/EditorMainWindow.h"
#include "Editor/EditorPayload.h"
#include "Platform/Paths.h"

#include <algorithm>
#include <optional>
#include <system_error>
#include <utility>
#include <vector>

#include <imgui/imgui.h>

using namespace ou;


namespace {

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

} // namespace



ou::ProjectWindow::ProjectWindow() {
	SetTitle("Project");
}


void ou::ProjectWindow::OnGui() {
	constexpr auto kTableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;
	if (!ImGui::BeginTable("##Project", 2, kTableFlags, ImGui::GetContentRegionAvail())) { return; }

	ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 200.0f);
	ImGui::TableSetupColumn("Contents", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableNextRow();

	ImGui::TableSetColumnIndex(0);
	if (ImGui::BeginChild("##Folders")) {
		for (const auto &root : FindRoots()) {
			DrawFolderTree(root.name, root.fullPath);
		}
	}
	ImGui::EndChild();

	ImGui::TableSetColumnIndex(1);
	if (ImGui::BeginChild("##Contents")) {
		DrawContents();
	}
	ImGui::EndChild();

	ImGui::EndTable();
}


void ou::ProjectWindow::DrawFolderTree(const std::string &assetPath, const std::filesystem::path &fullPath) {
	const auto subFolders = ListEntries(fullPath, true);

	auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (assetPath == selectedFolder_) { flags |= ImGuiTreeNodeFlags_Selected; }
	if (subFolders.empty()) { flags |= ImGuiTreeNodeFlags_Leaf; }

	const auto isOpen = ImGui::TreeNodeEx(assetPath.c_str(), flags, "%s", ToUtf8(fullPath.filename()).c_str());
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
	}
}


void ou::ProjectWindow::DrawFileItem(const std::string &assetPath, const std::filesystem::path &fullPath) {
	const auto isModel = IsModelFile(fullPath);

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
	flags |= isModel ? ImGuiTreeNodeFlags_OpenOnArrow : (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
	if (selectedItem_ == EditorPayload::EncodeAsset(assetPath, {})) { flags |= ImGuiTreeNodeFlags_Selected; }

	const auto isOpen = ImGui::TreeNodeEx(assetPath.c_str(), flags, "%s", ToUtf8(fullPath.filename()).c_str());
	HandleItem(assetPath, {});

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

	if (subAsset.empty() && IsSceneFile(assetPath) &&
		ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		EditorMainWindow::RequestOpenScene(assetPath);
	}

	if (ImGui::BeginDragDropSource()) {
		ImGui::SetDragDropPayload(EditorPayload::kAsset, payload.c_str(), payload.size() + 1);
		ImGui::Text("%s%s%s", assetPath.c_str(), subAsset.empty() ? "" : " : ", subAsset.c_str());
		ImGui::EndDragDropSource();
	}
}
