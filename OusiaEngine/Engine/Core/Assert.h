#pragma once

#include "Log.h"
#include <format>
#include <source_location>
#include <utility>


namespace ou {

inline void ReportAssertFailure(const char *expression, const std::source_location &location = std::source_location::current()) {
	Log::Write(LogLevel::Error, location, std::format("表明に失敗しました: {}", expression));
	FlushLog();
}

template <typename... Args>
void ReportAssertFailure(const char *expression, const std::source_location &location, std::format_string<Args...> text, Args &&...args) {
	Log::Write(LogLevel::Error, location, std::format("表明に失敗しました: {} ({})", expression, std::format(text, std::forward<Args>(args)...)));
	FlushLog();
}

} // namespace ou


#ifdef NDEBUG

#define ASSERT(condition, ...) ((void)sizeof(!(condition)))
#define VERIFY(condition, ...) ((void)(condition))

#else

#define ASSERT(condition, ...)                                                                                \
	do {                                                                                                      \
		if (!(condition)) [[unlikely]] {                                                                      \
			::ou::ReportAssertFailure(#condition, std::source_location::current() __VA_OPT__(, __VA_ARGS__)); \
			__debugbreak();                                                                                   \
		}                                                                                                     \
	} while (false)

#define VERIFY(condition, ...) ASSERT(condition __VA_OPT__(, __VA_ARGS__))

#endif
