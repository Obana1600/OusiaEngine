#include "Log.h"
#include "Core/StringUtil.h"
#include "Diagnostics/BuildInfo.h"
#include "Platform/Paths.h"

#include <Windows.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/dup_filter_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>


namespace {

std::shared_ptr<spdlog::logger> gLogger;

auto gMinLevel = ou::LogLevel::Info;

std::atomic<ou::LogListener> gListener = nullptr;

constexpr int kMaxLogFiles = 10;

constexpr auto kDuplicateWindow = std::chrono::seconds(2);

spdlog::level::level_enum ToSpdLogLevel(ou::LogLevel level) {
	switch (level) {
		case ou::LogLevel::Debug:
			return spdlog::level::debug;
		case ou::LogLevel::Info:
			return spdlog::level::info;
		case ou::LogLevel::Warning:
			return spdlog::level::warn;
		case ou::LogLevel::Error:
			return spdlog::level::err;
	}
	return spdlog::level::info;
}

std::wstring ToWString(ou::LogLevel level) {
	switch (level) {
		case ou::LogLevel::Debug:
			return L"Debug";
		case ou::LogLevel::Info:
			return L"Info";
		case ou::LogLevel::Warning:
			return L"Warning";
		case ou::LogLevel::Error:
			return L"Error";
	}
	return L"";
}

std::wstring MakeLogFileName() {
	const auto now = std::chrono::system_clock::now();
	const auto local = std::chrono::current_zone()->to_local(now);
	return std::format(L"{:%Y-%m-%d_%H-%M-%S}.log", std::chrono::floor<std::chrono::milliseconds>(local));
}

} // namespace



bool ou::IsLogEnabled(LogLevel level) { return static_cast<int>(level) >= static_cast<int>(gMinLevel); }


void ou::FlushLog() {
	if (gLogger) {
		gLogger->flush();
	}
}


void ou::SetLogListener(LogListener listener) {
	gListener = listener;
}


void ou::Log::Write(LogLevel level, const std::source_location &location, std::string_view message, const ScriptLocation &script) {
	const auto isScript = !script.file.empty();

	// コンソールなどに渡す（spdlog の重複を捨てるフィルタより前。Collapse で数えられるように、すべて渡す）
	if (const auto listener = gListener.load()) {
		listener({
			.level = level,
			.message = message,
			.file = isScript ? script.file : std::string_view(location.file_name()),
			.line = isScript ? script.line : static_cast<int>(location.line()),
			.isScript = isScript,
		});
	}

	if (!gLogger) {
		auto wide = L"[" + ToWString(level) + L"] " + Utf8ToWide(message);
		OutputDebugStringW(wide.c_str());
		OutputDebugStringW(L"\n");
		return;
	}

	// スクリプトなら、ファイルに残す場所もスクリプトにする（呼び出した C++ の場所は役に立たない）
	const std::string scriptFile(script.file);
	const auto loc = isScript
		? spdlog::source_loc{scriptFile.c_str(), script.line, ""}
		: spdlog::source_loc{location.file_name(), static_cast<int>(location.line()), location.function_name()};

	gLogger->log(loc, ToSpdLogLevel(level), "{}", message);
}


ou::LogSystem::LogSystem(const std::filesystem::path &logDirectory, std::span<const std::string> arguments) {
#ifdef _DEBUG
	gMinLevel = LogLevel::Debug;
#else
	gMinLevel = LogLevel::Info;
#endif

	auto filter = std::make_shared<spdlog::sinks::dup_filter_sink_mt>(kDuplicateWindow);

#if defined(_DEBUG) || defined(WITH_EDITOR)
	filter->add_sink(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#endif

	std::error_code ec;
	std::filesystem::create_directories(logDirectory, ec);
	RemoveOldFiles(logDirectory, L".log", kMaxLogFiles);

	try {
		const auto file = logDirectory / MakeLogFileName();
		filter->add_sink(std::make_shared<spdlog::sinks::basic_file_sink_mt>(file.wstring(), true));
	} catch (const spdlog::spdlog_ex &) {
	}

	gLogger = std::make_shared<spdlog::logger>("Ousia", filter);
	gLogger->set_level(spdlog::level::trace);
	gLogger->flush_on(spdlog::level::warn);
	gLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] %v");

	Log::Info("OusiaEngine 起動: 構成={}  ビルド={}", BuildConfigurationName(), BuildTimestamp());

	std::string joined;
	for (const auto &argument : arguments) {
		joined += argument;
		joined += ' ';
	}
	Log::Info("コマンドライン引数: {}", joined.empty() ? "(なし)" : joined);
}


ou::LogSystem::~LogSystem() {
	if (gLogger) {
		Log::Info("OusiaEngine 終了");
		gLogger->flush();
	}
	gLogger.reset();
}
