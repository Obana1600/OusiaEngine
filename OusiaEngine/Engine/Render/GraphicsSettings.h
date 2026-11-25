#pragma once

#include "Core/Object.h"

#include <memory>


namespace ou {

// 前方宣言
class VolumeProfile;

class GraphicsSettings final: public Object {
	friend class Engine;
	friend class VolumeManager;
	friend class ProjectSettingsWindow;

public: // メンバ関数

	void Serialize(Serializer &serializer) override;


private: // メンバ変数

	std::shared_ptr<VolumeProfile> defaultVolumeProfile_;


private: // 静的メンバ関数

	static GraphicsSettings &GetInstance();

	static void Load();
	static void Save();
	static void Unload();
};

} // namespace ou
