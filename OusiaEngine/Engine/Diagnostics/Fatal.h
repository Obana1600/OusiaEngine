#pragma once

#include <Windows.h>
#include <source_location>
#include <string>
#include <string_view>


namespace ou {

	// ログに出力しクラッシュさせる
	[[noreturn]]
	void FatalError(std::string_view message, const std::source_location &location = std::source_location::current());

	// HRESULT のメッセージを string に変換する
	std::string DescribeHResult(HRESULT hr);

	// HRESULT が成功しなければ、ログに出力しクラッシュさせる
	void CheckHResult(HRESULT hr, std::string_view what, const std::source_location &location = std::source_location::current());

} // namespace ou
