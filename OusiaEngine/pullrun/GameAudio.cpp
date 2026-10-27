#include "GameAudio.h"
#include "Asset/AssetDatabase.h"
#include "Audio/AudioClip.h"
#include "Audio/AudioSource.h"
#include "Core/Time.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"

using namespace ou;


namespace {

constexpr const char *kBgmPaths[] = {
	"Assets/BGM/title.mp3",
	"Assets/BGM/boss_normal.mp3",
	"Assets/BGM/boss_hard.mp3",
	"Assets/BGM/clear.mp3",
	"Assets/BGM/gameover.mp3",
};
static_assert(std::size(kBgmPaths) == static_cast<size_t>(Bgm::Count));

struct SeSetting {
	const char *path;
	// その音だけの音量（全体の音量 kSeVolume に掛ける）
	float volume;
	// 次に同じ音を鳴らせるまでの間（秒）
	float cooldown;
};

// 頻繁に鳴る弾と跳ねる音は小さく、たまにしか鳴らない節目の音は大きめにする
constexpr SeSetting kSeSettings[] = {
	{"Assets/SE/boss_bigbullet.wav", 0.7f, 0.0f},
	{"Assets/SE/boss_charge.wav", 0.6f, 0.2f},
	{"Assets/SE/boss_followbullet.wav", 0.35f, 0.1f},
	{"Assets/SE/boss_powerup.wav", 0.9f, 0.0f},
	{"Assets/SE/boss_smallbullet.wav", 0.25f, 0.1f},
	{"Assets/SE/player_bound.wav", 0.4f, 0.08f},
	{"Assets/SE/player_dash.wav", 0.6f, 0.05f},
	{"Assets/SE/player_dealdamage.wav", 0.7f, 0.05f},
	{"Assets/SE/player_dealdamage_maxcharge.wav", 0.8f, 0.05f},
	{"Assets/SE/player_takedamage.wav", 0.8f, 0.1f},
	{"Assets/SE/system_click.wav", 0.7f, 0.0f},
};
static_assert(std::size(kSeSettings) == static_cast<size_t>(Se::Count));

// SE 全体と BGM の音量
constexpr float kSeVolume = 0.6f;
constexpr float kBgmVolume = 0.4f;

} // namespace



void ou::GameAudio::EnsureCreated(Scene &scene) {
	if (sInstance_) { return; }
	auto *object = scene.CreateGameObject("Game Audio");
	object->AddComponent<GameAudio>();
	Object::DontDestroyOnLoad(object);
}


void ou::GameAudio::PlayBgm(Bgm bgm, bool loop) {
	auto *self = sInstance_;
	if (!self || (self->currentBgm_ == bgm && self->bgmSource_->IsPlaying())) { return; }
	self->currentBgm_ = bgm;
	self->bgmSource_->Stop();
	self->bgmSource_->SetClip(self->bgmClips_[static_cast<size_t>(bgm)]);
	self->bgmSource_->SetLoop(loop);
	self->bgmSource_->Play();
}


void ou::GameAudio::PlaySe(Se se, float volume) {
	auto *self = sInstance_;
	if (!self) { return; }
	const auto index = static_cast<size_t>(se);
	if (self->seCooldowns_[index] > 0.0f || !self->seClips_[index]) { return; }
	self->seCooldowns_[index] = kSeSettings[index].cooldown;
	self->seSource_->PlayOneShot(self->seClips_[index], volume * kSeSettings[index].volume);
}


void ou::GameAudio::Awake() {
	sInstance_ = this;

	bgmSource_ = GetGameObject()->AddComponent<AudioSource>();
	bgmSource_->SetPlayOnAwake(false);
	bgmSource_->SetSubmixType(Audio::SubmixType::Music);
	bgmSource_->SetVolume(kBgmVolume);

	seSource_ = GetGameObject()->AddComponent<AudioSource>();
	seSource_->SetPlayOnAwake(false);
	seSource_->SetVolume(kSeVolume);

	for (size_t i = 0; i < bgmClips_.size(); i++) { bgmClips_[i] = AssetDatabase::LoadAssetAtPath<AudioClip>(kBgmPaths[i]); }
	for (size_t i = 0; i < seClips_.size(); i++) { seClips_[i] = AssetDatabase::LoadAssetAtPath<AudioClip>(kSeSettings[i].path); }
}


void ou::GameAudio::Update() {
	// ヒットストップ中も同じ間隔にする
	const auto dt = Time::GetUnscaledDeltaTime();
	for (auto &cooldown : seCooldowns_) { cooldown -= dt; }
}


void ou::GameAudio::OnDestroy() {
	if (sInstance_ == this) { sInstance_ = nullptr; }
}
