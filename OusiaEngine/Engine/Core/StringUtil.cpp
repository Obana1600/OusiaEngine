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


std::u32string ou::Utf8ToUtf32(std::string_view utf8) {
	std::u32string result;
	result.reserve(utf8.size());

	for (size_t i = 0; i < utf8.size();) {
		const auto lead = static_cast<uint8_t>(utf8[i]);
		size_t length = 0;
		char32_t code = 0;

		// 先頭バイトから文字のバイト数を調べる
		if (lead < 0x80) {
			length = 1;
			code = lead;
		} else if ((lead & 0xE0) == 0xC0) {
			length = 2;
			code = lead & 0x1F;
		} else if ((lead & 0xF0) == 0xE0) {
			length = 3;
			code = lead & 0x0F;
		} else if ((lead & 0xF8) == 0xF0) {
			length = 4;
			code = lead & 0x07;
		}

		// 続きのバイトから文字コードを組み立てる
		bool isValid = (length != 0) && (i + length <= utf8.size());
		for (size_t k = 1; isValid && k < length; k++) {
			const auto next = static_cast<uint8_t>(utf8[i + k]);
			if ((next & 0xC0) != 0x80) {
				isValid = false;
				break;
			}
			code = (code << 6) | (next & 0x3F);
		}

		// 不正な文字なら置換文字を追加する
		if (!isValid) {
			result.push_back(U'\uFFFD');
			i++;
			continue;
		}

		result.push_back(code);
		i += length;
	}

	return result;
}


std::string ou::ToUtf8(const std::filesystem::path &path) {
	std::u8string u8 = path.u8string();
	return std::string(u8.begin(), u8.end());
}
