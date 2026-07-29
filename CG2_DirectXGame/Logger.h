#pragma once

#include <spdlog/spdlog.h>

using Logger = std::shared_ptr<spdlog::logger>;


enum class LoggerFlag : uint8_t {
	None = 0,				 // フラグなし
	MakeFile = 1 << 0,		 // ログファイルを生成
	UseDebugString = 1 << 1, // デバッグ文字列を使用
	UseConsole = 1 << 2,	 // コンソール出力を使用
};

LoggerFlag operator|(LoggerFlag a, LoggerFlag b);

bool operator&(LoggerFlag a, LoggerFlag b);


Logger GetLogger(const std::string &name, LoggerFlag flag = LoggerFlag::MakeFile | LoggerFlag::UseDebugString);