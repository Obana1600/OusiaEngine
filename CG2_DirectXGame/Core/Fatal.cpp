#include "Fatal.h"
#include "Log.h"
#include "Paths.h"

#include <format>


void ou::FatalError(std::string_view message, const std::source_location &location) {
	Log::Write(LogLevel::Error, location, std::format("致命的なエラー: {}", message));
	FlushLog();

	// デバッグ中ならその場で止める
	if (IsDebuggerPresent()) {
		__debugbreak();
	}

#ifdef NDEBUG
	// ユーザに理由を伝える
	MessageBoxW(nullptr, Utf8ToWide(message).c_str(), L"OusiaEngine", MB_OK | MB_ICONERROR);
#endif

	// 終了コード2 で終了する
	TerminateProcess(GetCurrentProcess(), 2);
}


std::string ou::DescribeHResult(HRESULT hr) {
	wchar_t *buffer = nullptr;

	// HRESULTからエラーメッセージを取りだす
	const auto length = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, static_cast<DWORD>(hr), 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr
	);

	auto text = (length > 0) ? WideToUtf8(std::wstring_view(buffer, length)) : "(説明なし)";
	LocalFree(buffer);

	// 末尾の改行を取り除く
	while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
		text.pop_back();
	}

	return std::format("0x{:08x} {}", static_cast<uint32_t>(hr), text);
}


void ou::CheckHResult(HRESULT hr, std::string_view what, const std::source_location &location) {
	if (SUCCEEDED(hr)) [[likely]] {
		return;
	}

	FatalError(std::format("{} に失敗しました: {}", what, DescribeHResult(hr)), location);
}
