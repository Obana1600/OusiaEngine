#pragma once

#include <Windows.h>
#include <cstdlib>
#include <exception>
#include <filesystem>


namespace ou {

class CrashHandler {
public: // メンバ関数

	explicit CrashHandler(const std::filesystem::path &dumpDirectory);
	~CrashHandler();


private: // メンバ変数

	LPTOP_LEVEL_EXCEPTION_FILTER prevFilter_ = nullptr;
	std::terminate_handler prevTerminate_ = nullptr;
	_purecall_handler prevPureCall_ = nullptr;
	_invalid_parameter_handler prevInvalidParameter_ = nullptr;
	void (*prevAbort_)(int) = nullptr;


private: // コピー禁止

	CrashHandler(const CrashHandler &) = delete;
	CrashHandler &operator=(const CrashHandler &) = delete;
};

} // namespace ou