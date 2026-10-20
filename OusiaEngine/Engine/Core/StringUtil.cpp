#include "StringUtil.h"

#include <Windows.h>


std::wstring ou::Utf8ToWide(std::string_view utf8) {
	if (utf8.empty()) {
		return {};
	}

	// 長さを測る
	auto length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
	if (length == 0) {
		return {};
	}

	// 実際に変換する
	std::wstring result(static_cast<size_t>(length), L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), result.data(), length);

	return result;
}


std::string ou::WideToUtf8(std::wstring_view wide) {
	if (wide.empty()) {
		return {};
	}

	// 長さを測る
	auto length = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
	if (length == 0) {
		return {};
	}

	// 実際に変換する
	std::string result(static_cast<size_t>(length), '\0');
	WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), result.data(), length, nullptr, nullptr);

	return result;
}


std::string ou::ToUtf8(const std::filesystem::path &path) {
	std::u8string u8 = path.u8string();
	return std::string(u8.begin(), u8.end());
}
