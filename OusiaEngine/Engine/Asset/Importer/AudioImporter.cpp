#include "AudioImporter.h"
#include "Audio/AudioClip.h"
#include "Audio/XAudio2/Audio.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"

#include <utility>

using namespace ou;


ou::AudioImporter::AudioImporter(std::string assetPath, std::filesystem::path fullPath)
	: AssetImporter(std::move(assetPath), std::move(fullPath)) {}


std::shared_ptr<Object> ou::AudioImporter::Import() {
	return ImportAudioClip();
}


std::shared_ptr<AudioClip> ou::AudioImporter::ImportAudioClip() {
	// 読み込み層のサウンドを作成
	Audio::Sound sound;
	if (!Audio::CreateSound(GetFullPath(), &sound)) {
		Log::Error("オーディオクリップを作れませんでした: {}", ToUtf8(GetFullPath()));
		return nullptr;
	}

	// アセット層のクリップを作成
	auto clip = std::make_shared<AudioClip>(std::move(sound));
	clip->SetName(GetName());
	return clip;
}
