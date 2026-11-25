#pragma once

#include "PostProcess/VolumeStack.h"
#include "Scene/LayerMask.h"

#include <vector>


namespace ou {

// 前方宣言
class Volume;

class VolumeManager {
	friend class Volume;
	friend class Engine;

public: // 静的メンバ関数

	VolumeManager() = delete;

	static VolumeStack &GetStack();


private: // 静的メンバ変数

	static inline std::vector<Volume *> sVolumes_;


private: // 静的メンバ関数

	static void Register(Volume *volume);
	static void Unregister(Volume *volume);

	static void Update(LayerMask layerMask);
};

} // namespace ou
