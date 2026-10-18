#pragma once

#include "Scene/Component.h"


namespace ou {

// 3d 音響の作成
class AudioListener: public Component {
public: // 静的メンバ関数

	static void SetVolume(float volume);
	static float GetVolume();
};

} // namespace ou
