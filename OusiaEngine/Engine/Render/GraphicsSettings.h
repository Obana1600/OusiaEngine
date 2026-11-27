#pragma once

#include "Core/Object.h"
#include "PostProcess/FullScreenPassRendererFeature.h"

#include <cstdint>
#include <memory>
#include <vector>


namespace ou {

// 前方宣言
class VolumeProfile;


enum class Downsampling : uint32_t {
	None,
	_2xBilinear, // 縦横 1/2
};

enum class ShadowResolution : uint32_t {
	_256,
	_512,
	_1024,
	_2048,
	_4096,
};


class GraphicsSettings final: public Object {
	friend class Engine;
	friend class SceneRenderTarget;
	friend class ProjectSettingsWindow;
	friend class VolumeManager;
	friend class PostProcessPass;
	friend class MainLightShadowCasterPass;

public: // メンバ関数

	void Serialize(Serializer &serializer) override;


private: // メンバ変数

	std::shared_ptr<VolumeProfile> defaultVolumeProfile_;
	std::vector<FullScreenPassRendererFeature> rendererFeatures_;

	Downsampling opaqueDownsampling_ = Downsampling::_2xBilinear;
	ShadowResolution mainLightShadowmapResolution_ = ShadowResolution::_2048;
	float shadowDistance_ = 50.0f;


private: // 静的メンバ関数

	static GraphicsSettings &GetInstance();

	static void Load();
	static void Save();
	static void Unload();
};

} // namespace ou
