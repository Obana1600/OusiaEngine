#pragma once

#include <string_view>


namespace ou {

// TODO: 仮実装。Develop 構成専用のマクロができたら判定を差し替える
constexpr std::string_view BuildConfigurationName() {
#if defined(_DEBUG)
	return "Debug";
#elif defined(_RELEASE)
	return "Release";
#else
	return "Develop";
#endif
}

constexpr std::string_view BuildTimestamp() {
	return __DATE__ " " __TIME__;
}

} // namespace ou
