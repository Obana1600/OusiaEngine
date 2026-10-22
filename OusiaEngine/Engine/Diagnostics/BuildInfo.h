#pragma once

#include <string_view>


namespace ou {

constexpr std::string_view BuildConfigurationName() {
#ifdef _DEBUG
	return "Debug";
#elif _DEVELOP
	return "Develop";
#else
	return "Release";
#endif
}

constexpr std::string_view BuildTimestamp() {
	return __DATE__ " " __TIME__;
}

} // namespace ou
