#include "Logger.h"

#include <filesystem>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>


LoggerFlag operator|(LoggerFlag a, LoggerFlag b) {
	return LoggerFlag(uint8_t(a) | uint8_t(b));
}


bool operator&(LoggerFlag a, LoggerFlag b) {
	return (uint8_t(a) & uint8_t(b)) != 0;
}


Logger GetLogger(const std::string &name, LoggerFlag flag) {
	auto existing = spdlog::get(name);
	if (existing) {
		return existing;
	}

	std::vector<spdlog::sink_ptr> sinks;

	if (flag & LoggerFlag::MakeFile) {
		std::filesystem::create_directories("Logs/" + name);

		auto now = std::chrono::system_clock::now();
		auto t = std::chrono::system_clock::to_time_t(now);
		std::tm tm;
		localtime_s(&tm, &t);

		std::ostringstream fileName;
		fileName << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".log";

		std::string logDir = "Logs/" + name + "/" + fileName.str();

		sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logDir, true));
	}

	if (flag & LoggerFlag::UseDebugString) {
		sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
	}

	if (flag & LoggerFlag::UseConsole) {
		sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	}


	auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
	spdlog::register_logger(logger);

#ifdef _DEBUG
	logger->set_level(spdlog::level::debug);
	logger->flush_on(spdlog::level::warn);
#endif
#ifdef _RELEASE
	logger->set_level(spdlog::level::info);
	logger->flush_on(spdlog::level::err);
#endif

	logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v");

	return logger;
}