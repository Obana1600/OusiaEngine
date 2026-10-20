#include "CrashHandler.h"
#include "Diagnostics/Log.h"
#include "Platform/Paths.h"

// clang-format off
#include <Windows.h>
#include <DbgHelp.h> // Windows.h より後ろに置く
#include <crtdbg.h>
#include <csignal>
#include <string>
// clang-format on

#pragma comment(lib, "dbghelp.lib")


namespace {

std::wstring gDumpDirectory;

constexpr int kMaxDumpFiles = 5;

LONG gHandling = 0;

void WriteMiniDump(EXCEPTION_POINTERS *exceptionInfo) {
	SYSTEMTIME time{};
	GetLocalTime(&time);

	wchar_t path[MAX_PATH]{};
	swprintf_s(path, L"%s\\%04d-%02d-%02d_%02d-%02d-%02d-%03d.dmp", gDumpDirectory.c_str(), time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

	auto file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE) {
		return;
	}

	MINIDUMP_EXCEPTION_INFORMATION info{};
	info.ThreadId = GetCurrentThreadId();
	info.ExceptionPointers = exceptionInfo;
	info.ClientPointers = FALSE;

	const auto type = static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory);

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, type, exceptionInfo ? &info : nullptr, nullptr, nullptr);

	CloseHandle(file);
}

[[noreturn]] void HandleCrash(const char *reason, EXCEPTION_POINTERS *exceptionInfo) {
	if (InterlockedExchange(&gHandling, 1) != 0) {
		TerminateProcess(GetCurrentProcess(), 3);
	}

	WriteMiniDump(exceptionInfo);

	ou::Log::Write(ou::LogLevel::Error, std::source_location::current(), reason);
	ou::FlushLog();

#ifdef NDEBUG
	MessageBoxW(nullptr, (L"予期しないエラーが発生しました。\n" + gDumpDirectory).c_str(), L"OusiaEngine", MB_OK | MB_ICONERROR);
#endif

	TerminateProcess(GetCurrentProcess(), 3);
}

LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS *info) { HandleCrash("未処理の構造化例外", info); }

void OnTerminate() {
	static std::string reason = "std::terminate が呼ばれました";
	if (auto current = std::current_exception()) {
		try {
			std::rethrow_exception(current);
		} catch (const std::exception &e) {
			reason = std::string("未処理の例外: ") + e.what();
		} catch (...) {
			reason = "未処理の例外（std::exception でない）";
		}
	}
	HandleCrash(reason.c_str(), nullptr);
}

void OnPureCall() { HandleCrash("純粋仮想関数が呼ばれました", nullptr); }

void OnInvalidParameter(const wchar_t *, const wchar_t *, const wchar_t *, unsigned int, uintptr_t) {
	HandleCrash("CRT に不正な引数が渡されました", nullptr);
}

void OnAbort(int) { HandleCrash("abort() が呼ばれました（assert の失敗など）", nullptr); }

} // namespace



ou::CrashHandler::CrashHandler(const std::filesystem::path &dumpDirectory) {
	std::error_code ec;
	std::filesystem::create_directories(dumpDirectory, ec);
	RemoveOldFiles(dumpDirectory, L".dmp", kMaxDumpFiles);
	gDumpDirectory = dumpDirectory.wstring();

	prevFilter_ = SetUnhandledExceptionFilter(OnUnhandledException);
	prevTerminate_ = std::set_terminate(OnTerminate);
	prevPureCall_ = _set_purecall_handler(OnPureCall);
	prevInvalidParameter_ = _set_invalid_parameter_handler(OnInvalidParameter);
	prevAbort_ = signal(SIGABRT, OnAbort);

	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

#ifdef _DEBUG
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
#endif
}


ou::CrashHandler::~CrashHandler() {
	SetUnhandledExceptionFilter(prevFilter_);
	std::set_terminate(prevTerminate_);
	_set_purecall_handler(prevPureCall_);
	_set_invalid_parameter_handler(prevInvalidParameter_);
	signal(SIGABRT, prevAbort_);

	gDumpDirectory.clear();
}
