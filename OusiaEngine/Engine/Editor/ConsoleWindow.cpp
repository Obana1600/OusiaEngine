#include "ConsoleWindow.h"
#include "Asset/AssetDatabase.h"
#include "Core/StringUtil.h"
#include "Editor/EditorPrefs.h"
#include "Editor/ExternalScriptEditor.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <format>
#include <functional>
#include <string_view>
#include <system_error>
#include <unordered_map>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

using namespace ou;


namespace {

constexpr const char *kCollapseKey = "Console.Collapse";
constexpr const char *kShowLogKey = "Console.ShowLog";
constexpr const char *kShowWarningKey = "Console.ShowWarning";
constexpr const char *kShowErrorKey = "Console.ShowError";

// 一覧と下の欄の間の、ドラッグで高さを変えるところ
constexpr float kSplitterThickness = 6.0f;

using Entry = LogEntries::Entry;


ImVec4 LevelColor(LogLevel level) {
	switch (level) {
		case LogLevel::Debug:
			return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
		case LogLevel::Warning:
			return {1.0f, 0.8f, 0.3f, 1.0f};
		case LogLevel::Error:
			return {1.0f, 0.45f, 0.45f, 1.0f};
		case LogLevel::Info:
			break;
	}
	return ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

// Unity と同じく 999 を超えたら 999+
std::string CountText(int count) {
	return count > 999 ? std::string("999+") : std::to_string(count);
}

// 英字だけ大文字小文字を区別しない（日本語はそのまま比べる）
bool ContainsIgnoreCase(std::string_view text, std::string_view pattern) {
	if (pattern.empty()) { return true; }
	const auto found = std::search(text.begin(), text.end(), pattern.begin(), pattern.end(), [](char a, char b) {
		return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
	});
	return found != text.end();
}

// 押すたびに切り替わるボタン。オンのときは押した色にする
bool ToggleButton(const char *label, bool &value) {
	if (value) { ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); }
	const auto isClicked = ImGui::Button(label);
	if (value) { ImGui::PopStyleColor(); }
	if (isClicked) { value = !value; }
	return isClicked;
}

float ButtonWidth(const char *label) {
	return ImGui::CalcTextSize(label, nullptr, true).x + ImGui::GetStyle().FramePadding.x * 2.0f;
}

// Collapse で「同じログ」とみなすもの（Unity と同じく、種類・本文・場所が同じ）
struct SameEntryHash {
	size_t operator()(const Entry *entry) const {
		auto hash = std::hash<std::string_view>{}(entry->message);
		hash ^= std::hash<std::string_view>{}(entry->file) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= std::hash<int>{}(entry->line * 4 + static_cast<int>(entry->level)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		return hash;
	}
};

struct SameEntryEqual {
	bool operator()(const Entry *a, const Entry *b) const {
		return a->level == b->level && a->line == b->line && a->message == b->message && a->file == b->file;
	}
};

// ログの場所を、開けるファイルのパスにする（スクリプトはアセットのパス、C++ はフルパス）
std::optional<std::filesystem::path> FindSourcePath(const Entry &entry) {
	if (entry.file.empty() || entry.line <= 0) { return std::nullopt; }
	if (entry.isScript) { return AssetDatabase::AssetPathToFullPath(entry.file); }

	std::error_code error;
	std::filesystem::path path(Utf8ToWide(entry.file));
	if (!std::filesystem::is_regular_file(path, error)) { return std::nullopt; }
	return path;
}

} // namespace



ou::ConsoleWindow::ConsoleWindow() {
	SetTitle("Console");

	isCollapsed_ = EditorPrefs::GetBool(kCollapseKey, false);
	showsLog_ = EditorPrefs::GetBool(kShowLogKey, true);
	showsWarning_ = EditorPrefs::GetBool(kShowWarningKey, true);
	showsError_ = EditorPrefs::GetBool(kShowErrorKey, true);
}


void ou::ConsoleWindow::OnGui() {
	DrawToolbar();
	RebuildRowsIfNeeded();

	// 一覧・境目・下の欄に分ける。どちらも 2 行ぶんは残す
	const auto spacing = ImGui::GetStyle().ItemSpacing.y;
	const auto available = ImGui::GetContentRegionAvail().y - kSplitterThickness - spacing * 2.0f;
	const auto minHeight = ImGui::GetFrameHeightWithSpacing() * 2.0f;
	detailHeight_ = std::clamp(detailHeight_, minHeight, std::max(minHeight, available - minHeight));

	DrawList(std::max(1.0f, available - detailHeight_));
	DrawSplitter();
	DrawDetail();
}


void ou::ConsoleWindow::DrawToolbar() {
	if (ImGui::Button("Clear")) {
		LogEntries::Clear();
		selectedSerial_.reset();
		selectedPath_.reset();
	}

	ImGui::SameLine();
	if (ToggleButton("Collapse", isCollapsed_)) {
		rowsVersion_ = UINT64_MAX;
		SavePrefs();
	}

	ImGui::SameLine();
	auto clearsOnPlay = LogEntries::IsClearOnPlay();
	if (ToggleButton("Clear on Play", clearsOnPlay)) {
		LogEntries::SetClearOnPlay(clearsOnPlay);
	}

	// 右側に種類ごとの数（Log は Debug と Info を合わせる）
	const auto logLabel = std::format("Log {}###Log", CountText(LogEntries::GetCount(LogLevel::Debug) + LogEntries::GetCount(LogLevel::Info)));
	const auto warningLabel = std::format("Warning {}###Warning", CountText(LogEntries::GetCount(LogLevel::Warning)));
	const auto errorLabel = std::format("Error {}###Error", CountText(LogEntries::GetCount(LogLevel::Error)));

	const auto itemSpacing = ImGui::GetStyle().ItemSpacing.x;
	const auto countsWidth = ButtonWidth(logLabel.c_str()) + ButtonWidth(warningLabel.c_str()) + ButtonWidth(errorLabel.c_str()) + itemSpacing * 2.0f;

	// 真ん中を検索欄にする
	ImGui::SameLine();
	ImGui::SetNextItemWidth(std::max(80.0f, ImGui::GetContentRegionAvail().x - countsWidth - itemSpacing));
	if (ImGui::InputTextWithHint("##Search", "検索", &search_)) {
		rowsVersion_ = UINT64_MAX;
	}

	auto isFilterChanged = false;
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, LevelColor(LogLevel::Info));
	isFilterChanged |= ToggleButton(logLabel.c_str(), showsLog_);
	ImGui::PopStyleColor();

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, LevelColor(LogLevel::Warning));
	isFilterChanged |= ToggleButton(warningLabel.c_str(), showsWarning_);
	ImGui::PopStyleColor();

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, LevelColor(LogLevel::Error));
	isFilterChanged |= ToggleButton(errorLabel.c_str(), showsError_);
	ImGui::PopStyleColor();

	if (isFilterChanged) {
		rowsVersion_ = UINT64_MAX;
		SavePrefs();
	}
}


void ou::ConsoleWindow::DrawList(float height) {
	if (!ImGui::BeginChild("##List", {0.0f, height}, ImGuiChildFlags_Borders)) {
		ImGui::EndChild();
		return;
	}

	const auto &entries = LogEntries::GetEntries();

	// ↑↓ で選ぶログを変える
	std::optional<int> scrollTarget;
	if (ImGui::IsWindowFocused() && !rows_.empty()) {
		const auto delta = ImGui::IsKeyPressed(ImGuiKey_UpArrow) ? -1 : ImGui::IsKeyPressed(ImGuiKey_DownArrow) ? 1 : 0;
		if (delta != 0) {
			const auto current = std::find_if(rows_.begin(), rows_.end(), [&](const Row &row) { return entries[row.index].serial == selectedSerial_; });
			const auto index = current == rows_.end() ? (delta > 0 ? 0 : static_cast<int>(rows_.size()) - 1)
													  : std::clamp(static_cast<int>(current - rows_.begin()) + delta, 0, static_cast<int>(rows_.size()) - 1);
			Select(entries[rows_[index].index]);
			scrollTarget = index;
		}
	}

	const auto &style = ImGui::GetStyle();
	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(rows_.size()));
	if (scrollTarget) { clipper.IncludeItemByIndex(*scrollTarget); }

	while (clipper.Step()) {
		for (auto i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
			const auto &row = rows_[i];
			const auto &entry = entries[row.index];
			ImGui::PushID(static_cast<int>(entry.serial));

			const auto startX = ImGui::GetCursorPosX();
			const auto rowScreen = ImGui::GetCursorScreenPos();
			const auto rowWidth = ImGui::GetContentRegionAvail().x;

			if (ImGui::Selectable("##Row", selectedSerial_ == entry.serial, ImGuiSelectableFlags_AllowDoubleClick)) {
				Select(entry);
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && selectedPath_) {
					ExternalScriptEditor::OpenFile(*selectedPath_, entry.line);
				}
			}
			if (scrollTarget == i && !ImGui::IsItemVisible()) {
				ImGui::SetScrollHereY(0.5f);
			}
			if (ImGui::BeginPopupContextItem()) {
				Select(entry);
				if (ImGui::MenuItem("Copy")) { ImGui::SetClipboardText(entry.message.c_str()); }
				ImGui::EndPopup();
			}

			// Collapse の数は右端に出し、本文はその手前で切る
			std::string countText;
			auto textRight = rowScreen.x + rowWidth;
			if (isCollapsed_) {
				countText = CountText(row.count);
				textRight -= ImGui::CalcTextSize(countText.c_str()).x + style.ItemSpacing.x;
			}

			ImGui::SameLine(startX);
			ImGui::TextDisabled("[%s]", entry.timeText.c_str());
			ImGui::SameLine();
			ImGui::PushClipRect(rowScreen, {textRight, rowScreen.y + ImGui::GetTextLineHeightWithSpacing()}, true);
			ImGui::PushStyleColor(ImGuiCol_Text, LevelColor(entry.level));
			ImGui::TextUnformatted(entry.message.data(), entry.message.data() + entry.firstLineLength);
			ImGui::PopStyleColor();
			ImGui::PopClipRect();

			if (isCollapsed_) {
				ImGui::SameLine(startX + rowWidth - ImGui::CalcTextSize(countText.c_str()).x);
				ImGui::TextDisabled("%s", countText.c_str());
			}

			ImGui::PopID();
		}
	}

	// いちばん下を見ていれば、新しいログに付いていく
	if (!scrollTarget && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::EndChild();
}


void ou::ConsoleWindow::DrawSplitter() {
	ImGui::InvisibleButton("##Splitter", {-FLT_MIN, kSplitterThickness});
	if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
	}
	if (ImGui::IsItemActive()) {
		detailHeight_ -= ImGui::GetIO().MouseDelta.y;
	}
}


void ou::ConsoleWindow::DrawDetail() {
	if (!ImGui::BeginChild("##Detail", {0.0f, 0.0f}, ImGuiChildFlags_Borders)) {
		ImGui::EndChild();
		return;
	}

	const auto *entry = FindSelected();
	if (!entry) {
		ImGui::TextDisabled("ログを選ぶと、ここに全文が出ます");
		ImGui::EndChild();
		return;
	}

	// 選べてコピーできるように、読み取り専用の入力欄に出す（ログごとに ID を変え、前のログの中身が残らないようにする）
	ImGui::PushID(static_cast<int>(entry->serial));
	auto &message = entry->message;
	ImGui::InputTextMultiline("##Message", const_cast<char *>(message.c_str()), message.size() + 1,
							  {-FLT_MIN, -ImGui::GetFrameHeightWithSpacing()}, ImGuiInputTextFlags_ReadOnly);
	ImGui::PopID();

	ImGui::AlignTextToFramePadding();
	if (!entry->file.empty()) {
		const auto location = std::format("{}:{}", entry->file, entry->line);
		if (selectedPath_) {
			if (ImGui::TextLink(location.c_str())) { ExternalScriptEditor::OpenFile(*selectedPath_, entry->line); }
		} else {
			ImGui::TextDisabled("%s", location.c_str());
		}
		ImGui::SameLine();
	}
	ImGui::TextDisabled("[%s]", entry->timeText.c_str());

	ImGui::EndChild();
}


void ou::ConsoleWindow::RebuildRowsIfNeeded() {
	if (rowsVersion_ == LogEntries::GetVersion()) { return; }
	rowsVersion_ = LogEntries::GetVersion();

	const auto &entries = LogEntries::GetEntries();
	rows_.clear();

	if (!isCollapsed_) {
		for (size_t i = 0; i < entries.size(); i++) {
			if (IsVisible(entries[i])) { rows_.push_back({i, 1}); }
		}
		return;
	}

	// 同じログは最初に出た行にまとめて数える
	std::unordered_map<const Entry *, size_t, SameEntryHash, SameEntryEqual> firstRows;
	for (size_t i = 0; i < entries.size(); i++) {
		if (!IsVisible(entries[i])) { continue; }

		const auto [found, isInserted] = firstRows.try_emplace(&entries[i], rows_.size());
		if (isInserted) {
			rows_.push_back({i, 1});
		} else {
			rows_[found->second].count++;
		}
	}
}


bool ou::ConsoleWindow::IsVisible(const LogEntries::Entry &entry) const {
	switch (entry.level) {
		case LogLevel::Debug:
		case LogLevel::Info:
			if (!showsLog_) { return false; }
			break;
		case LogLevel::Warning:
			if (!showsWarning_) { return false; }
			break;
		case LogLevel::Error:
			if (!showsError_) { return false; }
			break;
	}
	return ContainsIgnoreCase(entry.message, search_);
}


const LogEntries::Entry *ou::ConsoleWindow::FindSelected() const {
	const auto &entries = LogEntries::GetEntries();
	if (!selectedSerial_ || entries.empty()) { return nullptr; }

	// serial は続き番号なので、先頭との差が添字になる
	const auto first = entries.front().serial;
	if (*selectedSerial_ < first || *selectedSerial_ - first >= entries.size()) { return nullptr; }
	return &entries[static_cast<size_t>(*selectedSerial_ - first)];
}


void ou::ConsoleWindow::Select(const LogEntries::Entry &entry) {
	if (selectedSerial_ == entry.serial) { return; }
	selectedSerial_ = entry.serial;
	selectedPath_ = FindSourcePath(entry);
}


void ou::ConsoleWindow::SavePrefs() const {
	EditorPrefs::SetBool(kCollapseKey, isCollapsed_);
	EditorPrefs::SetBool(kShowLogKey, showsLog_);
	EditorPrefs::SetBool(kShowWarningKey, showsWarning_);
	EditorPrefs::SetBool(kShowErrorKey, showsError_);
}
