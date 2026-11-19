#include "ProjectWindow.h"
#include "Asset/AssetDatabase.h"
#include "Asset/AssetImporter.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorApplication.h"
#include "Editor/EditorMainWindow.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/EditorPayload.h"
#include "Editor/ExternalScriptEditor.h"
#include "Editor/PrefabUtility.h"
#include "Editor/Selection.h"
#include "Platform/Paths.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Serialize/SceneSerializer.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstdint>
#include <cstring>
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
#include <imgui/imgui_internal.h> // BeginDragDropTargetCustom
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
	return !ToUtf8(path.filename()).starts_with('.') && LowerExtension(path) != ".meta" && !SceneSerializer::IsObjectsFolder(path);
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

bool IsPrefabFile(const std::string &assetPath) {
	return LowerExtension(std::filesystem::path(Utf8ToWide(assetPath))) == ".prefab";
}

// EngineAssets はエンジンのものなので、作る・書き換えるのは Assets の中だけ
bool IsInAssets(const std::string &assetPath) {
	return assetPath == "Assets" || assetPath.starts_with("Assets/");
}

// Unity の AssetDatabase.GenerateUniqueAssetPath と同じく、あれば「名前 1」「名前 2」…にする
std::optional<std::string> MakeUniqueAssetPath(const std::string &folder, const std::string &name, std::string_view extension) {
	const auto folderPath = ToFullPath(folder);
	if (!folderPath) { return std::nullopt; }

	std::error_code error;
	for (int i = 0; i < 1000; i++) {
		const auto fileName = (i == 0 ? name : std::format("{} {}", name, i)) + std::string(extension);
		if (!std::filesystem::exists(*folderPath / Utf8ToWide(fileName), error)) { return folder + "/" + fileName; }
	}
	return std::nullopt;
}

// ファイル名に使えない文字を _ にする（GameObject の名前は自由なので）
std::string ToFileName(std::string_view name) {
	std::string result(name);
	std::replace_if(result.begin(), result.end(), [](char c) { return std::string_view("\\/:*?\"<>|").find(c) != std::string_view::npos; }, '_');
	return result.empty() ? "New Prefab" : result;
}

// path が folder 自身か、その中か
bool IsSameOrInside(const std::string &path, const std::string &folder) {
	return path == folder || path.starts_with(folder + "/");
}

// Assets の中のもの（Assets 自身は除く）だけ消せる
bool IsDeletable(const std::string &assetPath) {
	return assetPath != "Assets" && IsInAssets(assetPath);
}

// ファイルとフォルダを Windows のごみ箱へ移す（Explorer で消したときと同じく、ごみ箱から戻せる）
bool MoveToRecycleBin(const std::vector<std::filesystem::path> &paths) {
	// SHFileOperation は、\0 で区切って最後を \0\0 で終える
	std::wstring from;
	for (const auto &path : paths) {
		from += std::filesystem::path(path).make_preferred().wstring();
		from += L'\0';
	}
	from += L'\0';

	SHFILEOPSTRUCTW operation{};
	operation.wFunc = FO_DELETE;
	operation.pFrom = from.c_str();
	operation.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	return SHFileOperationW(&operation) == 0 && !operation.fAnyOperationsAborted;
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
	auto isTreeFocused = false;
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
			isTreeFocused = ImGui::IsWindowFocused();
		}
		ImGui::EndChild();

		ImGui::TableSetColumnIndex(1);
		if (ImGui::BeginChild("##Contents")) {
			DrawContents();
		}
		ImGui::EndChild();

		ImGui::EndTable();
	}

	// Delete キーで、選んでいるものを消す（サブアセットは消せない）。F2 で名前を変える
	// フォルダツリーを選んでいるときは、ツリーで選んでいるフォルダが相手
	const auto isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::GetIO().WantTextInput;
	if (isFocused && isTreeFocused) {
		if (ImGui::IsKeyPressed(ImGuiKey_Delete)) { BeginDelete(selectedFolder_); }
		if (ImGui::IsKeyPressed(ImGuiKey_F2, false) && CanRename(selectedFolder_)) { BeginRename(selectedFolder_, true); }
	} else if (const auto selectedPath = Selection::GetActiveAssetPath(); isFocused && !selectedPath.empty() && Selection::GetActiveSubAsset().empty()) {
		if (ImGui::IsKeyPressed(ImGuiKey_Delete)) { BeginDelete(selectedPath); }
		if (ImGui::IsKeyPressed(ImGuiKey_F2, false) && CanRename(selectedPath)) { BeginRename(selectedPath); }
	}

	// 選んでいるものを、間を空けてもう一度クリックしたら名前を変える
	if (const auto path = slowClick_.Update(); path && renamingPath_.empty()) {
		if (Selection::IsAssetSelected(*path) && CanRename(*path)) { BeginRename(*path); }
	}
	if (const auto path = treeSlowClick_.Update(); path && renamingPath_.empty()) {
		if (*path == selectedFolder_ && CanRename(*path)) { BeginRename(*path, true); }
	}

	DrawCreatePopup();
	DrawDeletePopup();
}


void ou::ProjectWindow::DrawFolderTree(const std::string &assetPath, const std::filesystem::path &fullPath) {
	const auto subFolders = ListEntries(fullPath, true);

	auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (assetPath == selectedFolder_) { flags |= ImGuiTreeNodeFlags_Selected; }
	if (subFolders.empty()) { flags |= ImGuiTreeNodeFlags_Leaf; }

	if (revealSelectedFolder_ && selectedFolder_.starts_with(assetPath + "/")) {
		ImGui::SetNextItemOpen(true);
	}

	const auto isRenaming = isRenamingInTree_ && renamingPath_ == assetPath;
	if (isRenaming) { flags |= ImGuiTreeNodeFlags_AllowOverlap; }

	const auto labelX = ImGui::GetCursorPosX() + ImGui::GetTreeNodeToLabelSpacing();
	const auto isSelected = assetPath == selectedFolder_;
	const auto isOpen = ImGui::TreeNodeEx(assetPath.c_str(), flags, "%s", isRenaming ? "" : ToUtf8(fullPath.filename()).c_str());
	if (revealSelectedFolder_ && assetPath == selectedFolder_) {
		ImGui::SetScrollHereY();
	}

	if (isRenaming) {
		DrawRenameField(assetPath, labelX);
	} else {
		treeSlowClick_.TrackItem(assetPath, isSelected);
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			selectedFolder_ = assetPath;
		}

		// Assets の中のフォルダは、ドラッグでほかのフォルダへ動かせる
		if (IsDeletable(assetPath) && ImGui::BeginDragDropSource()) {
			const auto payload = EditorPayload::EncodeAsset(assetPath, {});
			ImGui::SetDragDropPayload(EditorPayload::kAssetFolder, payload.c_str(), payload.size() + 1);
			ImGui::Text("%s", assetPath.c_str());
			ImGui::EndDragDropSource();
		}
		if (ImGui::BeginDragDropTarget()) {
			AcceptDropOnFolder(assetPath);
			ImGui::EndDragDropTarget();
		}
		DrawFolderTreeMenu(assetPath);
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

	// 何もないところに落とすと、開いているフォルダに作る
	// 窓全体を受け手にする（重なったときは ImGui が小さいほうを選ぶので、フォルダや .prefab の行が勝つ）
	if (auto *window = ImGui::GetCurrentWindow(); ImGui::BeginDragDropTargetCustom(window->InnerRect, window->ID)) {
		AcceptDropOnFolder(currentFolder);
		ImGui::EndDragDropTarget();
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
	if (!isRenamingInTree_ && assetPath == renamingPath_) {
		DrawRenameRow(assetPath);
		return;
	}

	constexpr auto kFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	const auto isSelected = Selection::IsAssetSelected(assetPath);
	ImGui::TreeNodeEx(assetPath.c_str(), kFlags | (isSelected ? ImGuiTreeNodeFlags_Selected : 0), "%s/", name.c_str());

	slowClick_.TrackItem(assetPath, isSelected);
	if (ImGui::IsItemClicked()) {
		Selection::SetActiveAsset(assetPath);
	}
	if (IsDeletable(assetPath) && ImGui::BeginDragDropSource()) {
		const auto payload = EditorPayload::EncodeAsset(assetPath, {});
		ImGui::SetDragDropPayload(EditorPayload::kAssetFolder, payload.c_str(), payload.size() + 1);
		ImGui::Text("%s", assetPath.c_str());
		ImGui::EndDragDropSource();
	}
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		selectedFolder_ = assetPath;
		revealSelectedFolder_ = true;
	}
	DrawItemMenu(assetPath);
	if (ImGui::BeginDragDropTarget()) {
		AcceptDropOnFolder(assetPath);
		ImGui::EndDragDropTarget();
	}
}


void ou::ProjectWindow::DrawFileItem(const std::string &assetPath, const std::filesystem::path &fullPath) {
	if (!isRenamingInTree_ && assetPath == renamingPath_) {
		DrawRenameRow(assetPath);
		return;
	}

	const auto isModel = IsModelFile(fullPath);
	const auto isSelected = Selection::IsAssetSelected(assetPath);

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
	flags |= isModel ? ImGuiTreeNodeFlags_OpenOnArrow : (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
	if (isSelected) { flags |= ImGuiTreeNodeFlags_Selected; }

	const auto isOpen = ImGui::TreeNodeEx(assetPath.c_str(), flags, "%s", ToUtf8(fullPath.filename()).c_str());
	slowClick_.TrackItem(assetPath, isSelected);
	HandleItem(assetPath, {});
	DrawItemMenu(assetPath);
	// Unity と同じく、プレハブの上に落とすと置き換える
	if (IsPrefabFile(assetPath)) { AcceptGameObjectDrop(assetPath); }

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
		const auto isSelected = Selection::IsAssetSelected(assetPath, sub.name);
		ImGui::TreeNodeEx(key.c_str(), kFlags | (isSelected ? ImGuiTreeNodeFlags_Selected : 0), "%s", sub.name.c_str());
		HandleItem(assetPath, sub.name);
	}
}


void ou::ProjectWindow::HandleItem(const std::string &assetPath, const std::string &subAsset) {
	const auto payload = EditorPayload::EncodeAsset(assetPath, subAsset);

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
		Selection::SetActiveAsset(assetPath, subAsset);
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
	// 行は TreePush しないので、名前にパスを入れないと、すべての行が同じポップアップになる（開くと全部の行の分が並ぶ）
	if (!ImGui::BeginPopupContextItem(("##ItemMenu" + assetPath).c_str())) { return; }

	Selection::SetActiveAsset(assetPath);
	DrawCreateMenu();
	ImGui::Separator();

	const auto canOpen = IsScriptFile(assetPath) || IsSceneFile(assetPath);
	if (ImGui::MenuItem("Open", nullptr, false, canOpen)) { OpenAsset(assetPath); }
	if (ImGui::MenuItem("Show in Explorer")) {
		if (const auto fullPath = ToFullPath(assetPath)) { ShowInExplorer(*fullPath); }
	}
	ImGui::Separator();
	if (ImGui::MenuItem("Rename", "F2", false, CanRename(assetPath))) { BeginRename(assetPath); }
	if (ImGui::MenuItem("Delete", "Delete", false, IsDeletable(assetPath))) { BeginDelete(assetPath); }
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
			Selection::SetActiveAsset(creatingFolder_ + "/" + fileName);
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


void ou::ProjectWindow::AcceptGameObjectDrop(const std::string &target) {
	if (!ImGui::BeginDragDropTarget()) { return; }
	SaveDroppedGameObject(target);
	ImGui::EndDragDropTarget();
}


void ou::ProjectWindow::SaveDroppedGameObject(const std::string &target) {
	if (!IsInAssets(target)) { return; }
	const auto *payload = ImGui::AcceptDragDropPayload(EditorPayload::kGameObject);
	if (!payload) { return; }

	uint32_t instanceId = 0;
	std::memcpy(&instanceId, payload->Data, sizeof(instanceId));
	auto *gameObject = dynamic_cast<GameObject *>(Object::FindObjectFromInstanceId(instanceId));
	if (!gameObject) { return; }

	// .prefab の上なら置き換え、フォルダなら「GameObject の名前.prefab」を新しく作る
	const auto assetPath = IsPrefabFile(target) ? std::optional<std::string>(target) : MakeUniqueAssetPath(target, ToFileName(gameObject->GetName()), ".prefab");
	if (!assetPath) { return; }

	if (PrefabUtility::SaveAsPrefabAsset(gameObject, *assetPath)) {
		Selection::SetActiveAsset(*assetPath);
	}
}


bool ou::ProjectWindow::CanRename(const std::string &assetPath) const {
	// Play を止めたときに、Play 前のシーンのパスへ戻ってしまうので、Play 中は変えない
	return IsDeletable(assetPath) && !EditorApplication::IsPlaying();
}


void ou::ProjectWindow::BeginRename(const std::string &assetPath, bool inTree) {
	if (!CanRename(assetPath)) { return; }

	const auto fullPath = ToFullPath(assetPath);
	std::error_code error;
	if (!fullPath || !std::filesystem::exists(*fullPath, error)) { return; }

	// 行が見えるように、ツリーなら親を開き、一覧なら親のフォルダを出す
	if (inTree) {
		selectedFolder_ = assetPath;
		revealSelectedFolder_ = true;
	} else if (const auto parent = assetPath.substr(0, assetPath.find_last_of('/')); selectedFolder_ != parent) {
		selectedFolder_ = parent;
		revealSelectedFolder_ = true;
	}
	isRenamingInTree_ = inTree;

	const auto path = std::filesystem::path(Utf8ToWide(assetPath));
	const auto isFolder = std::filesystem::is_directory(*fullPath, error);
	renamingPath_ = assetPath;
	renamingName_ = ToUtf8(isFolder ? path.filename() : path.stem());
	renamingExtension_ = isFolder ? std::string() : ToUtf8(path.extension());
	focusesRename_ = true;
	renameInactiveFrames_ = 0;
	slowClick_.Cancel();
	Focus();
}


void ou::ProjectWindow::DrawRenameRow(const std::string &assetPath) {
	// 行と同じ並びに見えるように、選んだ行の上に名前の欄を重ねる
	constexpr auto kFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth |
							ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_AllowOverlap;
	const auto labelX = ImGui::GetCursorPosX() + ImGui::GetTreeNodeToLabelSpacing();
	ImGui::TreeNodeEx(assetPath.c_str(), kFlags, "%s", "");
	DrawRenameField(assetPath, labelX);
}


void ou::ProjectWindow::DrawRenameField(const std::string &assetPath, float labelX) {
	ImGui::SameLine(labelX);

	if (std::exchange(focusesRename_, false)) {
		ImGui::SetKeyboardFocusHere();
		ImGui::SetScrollHereY();
	}

	const auto extensionWidth = renamingExtension_.empty() ? 0.0f : ImGui::CalcTextSize(renamingExtension_.c_str()).x + ImGui::GetStyle().ItemSpacing.x;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {ImGui::GetStyle().FramePadding.x, 0.0f});
	ImGui::SetNextItemWidth(std::max(ImGui::GetContentRegionAvail().x - extensionWidth, 40.0f));
	ImGui::InputText("##Rename", &renamingName_, ImGuiInputTextFlags_AutoSelectAll);
	ImGui::PopStyleVar();
	const auto isDeactivated = ImGui::IsItemDeactivated();
	const auto isActive = ImGui::IsItemActive();

	// 拡張子は変えないので、欄の外に出しておく
	if (!renamingExtension_.empty()) {
		ImGui::SameLine();
		ImGui::TextDisabled("%s", renamingExtension_.c_str());
	}

	const auto problem = FindRenameProblem();
	if (!problem.empty()) {
		ImGui::TextColored({1.0f, 0.4f, 0.4f, 1.0f}, "%s", problem.c_str());
	} else if (IsScriptFile(assetPath) && renamingName_ != ToUtf8(std::filesystem::path(Utf8ToWide(assetPath)).stem())) {
		// ファイル名をクラス名として探すので、中の class の名前と違うと見つからない（Unity と同じく、中身は書き換えない）
		ImGui::TextColored({1.0f, 0.8f, 0.3f, 1.0f}, "クラス名は書き換えません。ファイル名とクラス名が違うと、付けているコンポーネントは Missing になります");
	}

	if (isDeactivated) {
		const auto isEscape = ImGui::IsKeyPressed(ImGuiKey_Escape);
		const auto isEnter = ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter);
		if (isEscape) {
			EndRename(false);
		} else if (problem.empty()) {
			EndRename(true);
		} else if (isEnter) {
			// 変えられない名前で Enter を押したときは、直せるように欄に戻る
			focusesRename_ = true;
		} else {
			// ほかをクリックしたときは、元の名前のまま
			EndRename(false);
		}
		return;
	}

	// 入力が入らないまま（ウィンドウが隠れていたなど）残らないようにする
	renameInactiveFrames_ = (isActive || focusesRename_) ? 0 : renameInactiveFrames_ + 1;
	if (renameInactiveFrames_ > 2) { EndRename(false); }
}


std::string ou::ProjectWindow::FindRenameProblem() const {
	if (renamingName_.empty()) { return "名前を入れてください"; }
	if (renamingName_.find_first_of("\\/:*?\"<>|") != std::string::npos) { return "使えない文字があります（\\ / : * ? \" < > |）"; }
	if (renamingName_.back() == '.' || renamingName_.back() == ' ') { return "最後を . や空白にはできません"; }
	if (renamingName_ == "." || renamingName_ == "..") { return "この名前は使えません"; }

	const auto parent = renamingPath_.substr(0, renamingPath_.find_last_of('/'));
	const auto newPath = parent + "/" + renamingName_ + renamingExtension_;
	if (newPath == renamingPath_) { return {}; }

	// 大文字と小文字だけを変えるときは、Windows では同じものとして見つかるので、それは許す
	const auto oldFull = ToFullPath(renamingPath_);
	const auto newFull = ToFullPath(newPath);
	std::error_code error;
	if (!oldFull || !newFull) { return "パスが正しくありません"; }
	if (std::filesystem::exists(*newFull, error) && !std::filesystem::equivalent(*oldFull, *newFull, error)) {
		return "同じ名前のものがもうあります";
	}
	return {};
}


void ou::ProjectWindow::EndRename(bool commits) {
	const auto oldPath = std::exchange(renamingPath_, std::string());
	focusesRename_ = false;
	if (!commits || oldPath.empty()) { return; }

	const auto parent = oldPath.substr(0, oldPath.find_last_of('/'));
	const auto newPath = parent + "/" + renamingName_ + renamingExtension_;
	if (newPath == oldPath) { return; }

	if (const auto error = AssetDatabase::MoveAsset(oldPath, newPath); !error.empty()) {
		Log::Error("名前を変えられませんでした: {}", error);
		return;
	}
	AfterAssetMoved(oldPath, newPath);

	if (IsScriptFile(oldPath)) {
		const auto oldName = ToUtf8(std::filesystem::path(Utf8ToWide(oldPath)).stem());
		Log::Warning("スクリプトのファイル名を {} にしました。中の class {} も {} にしないと、付けているコンポーネントは Missing になります", renamingName_, oldName, renamingName_);
	}
}


void ou::ProjectWindow::DrawFolderTreeMenu(const std::string &assetPath) {
	if (!ImGui::BeginPopupContextItem(("##TreeMenu" + assetPath).c_str())) { return; }

	// Create は開いているフォルダに作るので、先にこのフォルダを開く
	selectedFolder_ = assetPath;
	DrawCreateMenu();
	ImGui::Separator();
	if (ImGui::MenuItem("Show in Explorer")) {
		if (const auto fullPath = ToFullPath(assetPath)) { ShowInExplorer(*fullPath); }
	}
	ImGui::Separator();
	if (ImGui::MenuItem("Rename", "F2", false, CanRename(assetPath))) { BeginRename(assetPath, true); }
	if (ImGui::MenuItem("Delete", "Delete", false, IsDeletable(assetPath))) { BeginDelete(assetPath); }
	ImGui::EndPopup();
}


bool ou::ProjectWindow::CanMove(const std::string &source, const std::string &folder) const {
	if (!IsDeletable(source) || !IsInAssets(folder) || EditorApplication::IsPlaying()) { return false; }

	// 今と同じフォルダ、自分自身、自分の中へは動かさない
	const auto parent = source.substr(0, source.find_last_of('/'));
	return parent != folder && !IsSameOrInside(folder, source);
}


void ou::ProjectWindow::AcceptDropOnFolder(const std::string &folder) {
	// Hierarchy から GameObject を落としたら、プレハブを作る
	SaveDroppedGameObject(folder);

	// ファイル・フォルダを落としたら、このフォルダへ動かす
	const auto *peek = ImGui::GetDragDropPayload();
	if (!peek) { return; }

	const char *type = nullptr;
	if (peek->IsDataType(EditorPayload::kAsset)) {
		type = EditorPayload::kAsset;
	} else if (peek->IsDataType(EditorPayload::kAssetFolder)) {
		type = EditorPayload::kAssetFolder;
	} else {
		return;
	}

	// 動かせないところでは受けない（枠が光らない）
	const auto source = EditorPayload::DecodeAsset(peek->Data, peek->DataSize);
	if (!source.subAsset.empty() || !CanMove(source.path, folder)) { return; }

	if (const auto *payload = ImGui::AcceptDragDropPayload(type)) {
		MoveAssetInto(source.path, folder);
	}
}


void ou::ProjectWindow::MoveAssetInto(const std::string &source, const std::string &folder) {
	if (!CanMove(source, folder)) { return; }

	const auto newPath = folder + "/" + source.substr(source.find_last_of('/') + 1);
	if (const auto error = AssetDatabase::MoveAsset(source, newPath); !error.empty()) {
		// 同じ名前のものがあるときなど。Unity と同じく、動かさずに知らせる
		Log::Error("動かせませんでした: {}", error);
		return;
	}
	AfterAssetMoved(source, newPath);
}


void ou::ProjectWindow::AfterAssetMoved(const std::string &oldPath, const std::string &newPath) {
	EditorSceneManager::NotifyAssetMoved(oldPath, newPath);

	// 選んでいるもの・開いているフォルダが、動かしたものかその中なら、新しいパスにする
	const auto movePath = [&](const std::string &path) { return newPath + path.substr(oldPath.size()); };
	if (const auto selected = Selection::GetActiveAssetPath(); IsSameOrInside(selected, oldPath)) {
		Selection::SetActiveAsset(movePath(selected), Selection::GetActiveSubAsset());
	}
	if (IsSameOrInside(selectedFolder_, oldPath)) {
		selectedFolder_ = movePath(selectedFolder_);
		revealSelectedFolder_ = true;
	}
}


void ou::ProjectWindow::BeginDelete(const std::string &assetPath) {
	if (!IsDeletable(assetPath)) { return; }
	deletingPath_ = assetPath;
	opensDeletePopup_ = true;
}


void ou::ProjectWindow::DrawDeletePopup() {
	constexpr const char *kDeletePopup = "Delete";
	if (std::exchange(opensDeletePopup_, false)) {
		ImGui::OpenPopup(kDeletePopup);
	}
	if (!ImGui::BeginPopupModal(kDeletePopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) { return; }

	ImGui::Text("%s を削除しますか？", deletingPath_.c_str());
	ImGui::TextDisabled("Windows のごみ箱に移します（ごみ箱から戻せます）。これを指していた欄は None になります");

	// 開いているシーンは消せない（消すと、保存したときに同じパスへ書き戻される）
	const auto *scene = SceneManager::GetActiveScene();
	const auto isOpenScene = scene && !scene->GetPath().empty() && IsSameOrInside(scene->GetPath(), deletingPath_);
	if (isOpenScene) {
		ImGui::TextColored({1.0f, 0.4f, 0.4f, 1.0f}, "開いているシーン（%s）は削除できません", scene->GetPath().c_str());
	}

	ImGui::BeginDisabled(isOpenScene);
	const auto isDeleteClicked = ImGui::Button("削除");
	ImGui::EndDisabled();

	if (isDeleteClicked) {
		// .meta も一緒に消す（残すと「相手のファイルがない .meta」になる）
		std::vector<std::filesystem::path> paths;
		if (const auto fullPath = ToFullPath(deletingPath_)) {
			std::error_code error;
			paths.push_back(*fullPath);
			auto metaPath = *fullPath;
			metaPath += L".meta";
			if (std::filesystem::exists(metaPath, error)) { paths.push_back(metaPath); }
			if (const auto objects = SceneSerializer::ToObjectsFolder(*fullPath); SceneSerializer::IsObjectsFolder(objects)) { paths.push_back(objects); }
		}

		if (!paths.empty() && MoveToRecycleBin(paths)) {
			// GUID の表を作り直す（このパスの guid が引けなくなる）
			AssetDatabase::Refresh();
			if (IsSameOrInside(selectedFolder_, deletingPath_)) {
				selectedFolder_ = deletingPath_.substr(0, deletingPath_.find_last_of('/'));
			}
			if (IsSameOrInside(Selection::GetActiveAssetPath(), deletingPath_)) { Selection::Clear(); }
			Log::Info("ごみ箱に移しました: {}", deletingPath_);
		} else {
			Log::Error("削除できませんでした: {}", deletingPath_);
		}
		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine();
	if (ImGui::Button("キャンセル")) {
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();
}
