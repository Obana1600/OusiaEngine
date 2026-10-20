#pragma once

#include <filesystem>
#include <string>
#include <string_view>


namespace ou {

// UTF-8 -> UTF-16
std::wstring Utf8ToWide(std::string_view utf8);

// UTF-16 -> UTF-8
std::string WideToUtf8(std::wstring_view wide);

// filePath -> UTF-8
std::string ToUtf8(const std::filesystem::path &path);

} // namespace ou
