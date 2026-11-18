#pragma once

#include "Editor/EditorWindow.h"
#include "Editor/LogEntries.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>


namespace ou {

// Unity の Window > General > Console
// 上が一覧（1 行目だけ）、下が選んだログの全文と場所。ダブルクリックでその場所をエディタで開く
class ConsoleWindow final: public EditorWindow {
public: // メンバ関数

	ConsoleWindow();


protected: // メンバ関数

	void OnGui() override;


private: // インナークラス

	struct Row {
		// LogEntries::GetEntries() の添字（LogEntries の version が変わるまで有効）
		size_t index = 0;
		// Collapse のときの同じログの数
		int count = 1;
	};


private: // メンバ変数

	bool isCollapsed_ = false;
	bool showsLog_ = true;
	bool showsWarning_ = true;
	bool showsError_ = true;
	std::string search_;

	std::vector<Row> rows_;
	// rows_ を作ったときの LogEntries の version（UINT64_MAX なら作り直す）
	uint64_t rowsVersion_ = UINT64_MAX;

	std::optional<uint64_t> selectedSerial_;
	// 選んだログを開くファイル（開けないなら nullopt）
	std::optional<std::filesystem::path> selectedPath_;

	// 下の欄の高さ
	float detailHeight_ = 120.0f;


private: // メンバ関数

	void DrawToolbar();
	void DrawList(float height);
	void DrawSplitter();
	void DrawDetail();

	void RebuildRowsIfNeeded();
	bool IsVisible(const LogEntries::Entry &entry) const;

	const LogEntries::Entry *FindSelected() const;
	void Select(const LogEntries::Entry &entry);

	void SavePrefs() const;
};

} // namespace ou
