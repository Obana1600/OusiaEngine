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

	// 0~2（Default / Ignore Raycast / UI）は組み込みで、名前を変えられない
	static bool IsBuiltinLayer(int layer);
	// 組み込みのレイヤー・ほかのレイヤーと同じ名前なら変えずに false（空にするとレイヤーの名前をなくす）
	static bool SetLayerName(int layer, std::string_view name);
	// ProjectSettings/TagManager.json に書く
	static void SaveLayerNames();

	operator uint32_t() const;
};

} // namespace ou
