#pragma once

#include "Diagnostics/Log.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>


namespace ou {

// Unity の UnityEditor.LogEntries（internal）。コンソールに出すログの履歴
// ログはどのスレッドからも来るので、いったん pending_ に貯めて、メインスレッドで entries_ に移す
// Unity でも internal なので、関数は private にして使う側を friend にする
class LogEntries final {
	friend class ConsoleWindow;
	friend class EditorApplication;
	friend class Engine;

public: // 静的メンバ関数

	LogEntries() = delete;


public: // インナークラス

	// 中身は friend からしか取れないが、型はコンソールの中の関数でも使うので public にする
	struct Entry {
		// 0 から順に振る番号（古いものを捨てても変わらないので、選択を覚えるのに使う）
		uint64_t serial = 0;
		LogLevel level = LogLevel::Info;
		std::string message;
		// isScript ならアセットのパス、そうでなければ C++ のファイルのフルパス
		std::string file;
		int line = 0;
		bool isScript = false;
		std::chrono::system_clock::time_point time;
		// "12:34:56"（Update で作る）
		std::string timeText;
		// 1 行目の長さ（一覧には 1 行目だけ出す）
		size_t firstLineLength = 0;
	};


private: // 静的メンバ変数

	// これより古いものは捨てる
	static constexpr size_t kMaxEntries = 10000;

	static inline std::mutex sMutex_;
	static inline std::vector<Entry> sPending_;

	// ここからはメインスレッドだけで触る
	static inline std::deque<Entry> sEntries_;
	static inline uint64_t sNextSerial_ = 0;
	// 中身が変わるたびに増える（コンソールが表示する行を作り直すかどうかに使う）
	static inline uint64_t sVersion_ = 0;
	// LogLevel ごとの数
	static inline std::array<int, 4> sCounts_{};


private: // 静的メンバ関数

	// ログを受け取り始める（LogSystem より前に呼ぶと、起動時のログも拾える）
	static void Initialize();
	static void Finalize();

	// 届いたログを entries_ に移す（メインスレッドで毎フレーム）
	static void Update();

	static void Clear();

	// Unity の Clear on Play（Play に入るときに呼ぶ）
	static void ClearOnPlayIfEnabled();

	static bool IsClearOnPlay();
	static void SetClearOnPlay(bool enabled);

	static const std::deque<Entry> &GetEntries();
	static uint64_t GetVersion();
	static int GetCount(LogLevel level);

	static void OnLogMessage(const LogMessage &message);
};

} // namespace ou
