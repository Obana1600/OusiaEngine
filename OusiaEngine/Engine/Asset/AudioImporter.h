#pragma once

#include "Asset/AssetImporter.h"

#include <filesystem>
#include <memory>


namespace ou {

// 前方宣言
class AudioClip;

// TODO: ストリーミング再生
class AudioImporter: public AssetImporter {
public: // メンバ関数

	explicit AudioImporter(std::filesystem::path assetPath);

	std::shared_ptr<Object> Import() override;
	std::shared_ptr<AudioClip> ImportAudioClip();
};

} // namespace ou
