#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>


namespace ou {

struct LayerMask {
	static constexpr int kLayerCount = 32;

	uint32_t value = 0;

	static int NameToLayer(std::string_view layerName);
	static std::string LayerToName(int layer);
	static uint32_t GetMask(std::initializer_list<std::string_view> layerNames);

	static void LoadLayerNames();

	operator uint32_t() const;
};

} // namespace ou
