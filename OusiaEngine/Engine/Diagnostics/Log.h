#pragma once

#include <filesystem>
#include <format>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>


namespace ou {

//
enum class LogLevel {
	Debug,
	Info,
	Warning,
	Error
};


// C++ 以外（スクリプト）でログを出した場所。file はアセットのパス
struct ScriptLocation {
	std::string_view file;
	int line = 0;
};

// ログを受け取る側に渡すもの（Unity の Application.logMessageReceivedThreaded の引数）
struct LogMessage {
	LogLevel level = LogLevel::Info;
	std::string_view message;
	// isScript なら、アセットのパスとスクリプトの行。そうでなければ C++ のファイルと行
	std::string_view file;
	int line = 0;
	bool isScript = false;
};

// どのスレッドからも呼ばれる。中でログを出してはいけない
using LogListener = void (*)(const LogMessage &message);


namespace Detail {

// {} の数を数える
consteval size_t CountPlaceholders(std::string_view text) {
	size_t count = 0;
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] != '{') {
			continue;
		}
		if (i + 1 < text.size() && text[i + 1] == '{') {
			i++;
			continue;
		}
		count++;
	}
	return count;
}

template <typename... Args>
struct FormatWithLocation {
	std::format_string<Args...> text;
	std::source_location location;

	consteval FormatWithLocation(const char *literal, std::source_location loc = std::source_location::current())
		: text(literal), location(loc) {
		if (CountPlaceholders(literal) != sizeof...(Args)) {
			throw "書式の {} の数と、引数の数が合っていません";
		}
	}
};

} // namespace Detail


bool IsLogEnabled(LogLevel level);

void FlushLog();

// ログを受け取る関数を登録する（1 つだけ。nullptr で外す）
void SetLogListener(LogListener listener);


namespace Log {

// script を渡すと、場所は C++ ではなくスクリプトのものになる
void Write(LogLevel level, const std::source_location &location, std::string_view message, const ScriptLocation &script = {});

// 
template <typename... Args>
void Info(Detail::FormatWithLocation<std::type_identity_t<Args>...> format, Args &&...args) {
	if (!IsLogEnabled(LogLevel::Info)) {
		return;
	}

	Write(LogLevel::Info, format.location, std::format(format.text, std::forward<Args>(args)...));
}

// 
template <typename... Args>
void Debug(Detail::FormatWithLocation<std::type_identity_t<Args>...> format, Args &&...args) {
	if (!IsLogEnabled(LogLevel::Debug)) {
		return;
	}

	Write(LogLevel::Debug, format.location, std::format(format.text, std::forward<Args>(args)...));
}

// 
template <typename... Args>
void Warning(Detail::FormatWithLocation<std::type_identity_t<Args>...> format, Args &&...args) {
	if (!IsLogEnabled(LogLevel::Warning)) {
		return;
	}

	Write(LogLevel::Warning, format.location, std::format(format.text, std::forward<Args>(args)...));
}

// 
template <typename... Args>
void Error(Detail::FormatWithLocation<std::type_identity_t<Args>...> format, Args &&...args) {
	if (!IsLogEnabled(LogLevel::Error)) {
		return;
	}

	Write(LogLevel::Error, format.location, std::format(format.text, std::forward<Args>(args)...));
}

} // namespace Log


class LogSystem {
public: // メンバ関数

	explicit LogSystem(const std::filesystem::path &logDirectory, std::span<const std::string> arguments);
	~LogSystem();

	// コピーの禁止
	LogSystem(const LogSystem &) = delete;
	LogSystem &operator=(const LogSystem &) = delete;
};

} // namespace ou