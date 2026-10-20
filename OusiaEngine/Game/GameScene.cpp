#include "GameScene.h"
#include "Asset/AssetDatabase.h"
#include "Audio/AudioClip.h"
#include "Audio/AudioListener.h"
#include "Audio/AudioSource.h"
#include "Audio/XAudio2/Audio.h"
#include "Core/Time.h"
#include "Math/Mathf.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleSystemRenderer.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Texture.h"
#include "Render/Camera.h"
#include "Scene/Transform.h"

#include <imgui/imgui.h>

using namespace ou;


namespace {

#ifdef USE_IMGUI

// 「定数」と「2 つの値の間のランダム」を切り替えて編集する
void EditFloatRange(const char *label, FloatRange &range, float speed, float min, float max) {
	ImGui::PushID(label);

	bool isRandom = (range.min != range.max);
	if (ImGui::Checkbox("ランダム", &isRandom) && !isRandom) {
		range.max = range.min;
	}
	ImGui::SameLine();

	if (isRandom) {
		ImGui::DragFloatRange2(label, &range.min, &range.max, speed, min, max);
	} else if (ImGui::DragFloat(label, &range.min, speed, min, max)) {
		range.max = range.min;
	}

	ImGui::PopID();
}

void EditColorRange(const char *label, ColorRange &range) {
	ImGui::PushID(label);

	bool isRandom = !(range.min == range.max);
	if (ImGui::Checkbox("ランダム", &isRandom) && !isRandom) {
		range.max = range.min;
	}
	ImGui::SameLine();

	if (isRandom) {
		ImGui::ColorEdit4("##min", &range.min.r, ImGuiColorEditFlags_NoInputs);
		ImGui::SameLine();
		ImGui::ColorEdit4(label, &range.max.r, ImGuiColorEditFlags_NoInputs);
	} else if (ImGui::ColorEdit4(label, &range.min.r)) {
		range.max = range.min;
	}

	ImGui::PopID();
}

// AudioSource の再生の操作と設定。ヘッダが開いていれば true を返す
bool EditAudioSource(const char *label, AudioSource &source) {
	if (!ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) { return false; }
	ImGui::PushID(label);

	const auto &clip = source.GetClip();
	ImGui::Text("%s (%.1f 秒)  %s",
				clip ? clip->GetName().c_str() : "クリップなし",
				clip ? clip->GetLength() : 0.0f,
				source.IsPlaying() ? "再生中" : "停止中");

	if (ImGui::Button("Play")) { source.Play(); }
	ImGui::SameLine();
	if (ImGui::Button("Pause")) { source.Pause(); }
	ImGui::SameLine();
	if (ImGui::Button("UnPause")) { source.UnPause(); }
	ImGui::SameLine();
	if (ImGui::Button("Stop")) { source.Stop(); }

	auto volume = source.GetVolume();
	if (ImGui::SliderFloat("volume", &volume, 0.0f, 1.0f)) { source.SetVolume(volume); }
	auto pitch = source.GetPitch();
	if (ImGui::SliderFloat("pitch", &pitch, 0.1f, 3.0f)) { source.SetPitch(pitch); }
	auto loop = source.GetLoop();
	if (ImGui::Checkbox("loop（次の Play から）", &loop)) { source.SetLoop(loop); }
	ImGui::SameLine();
	auto mute = source.GetMute();
	if (ImGui::Checkbox("mute", &mute)) { source.SetMute(mute); }

	ImGui::PopID();
	return true;
}

#endif

} // namespace


void GameScene::Initialize() {
	scene_ = std::make_unique<Scene>();

	auto *cameraObject = scene_->CreateGameObject("Main Camera");
	cameraObject->AddComponent<Camera>();
	cameraObject->GetTransform()->SetLocalPosition({0.0f, 3.0f, -12.0f});
	cameraObject->GetTransform()->SetLocalEulerAngles({10.0f * Mathf::Deg2Rad, 0.0f, 0.0f});
	cameraObject->AddComponent<AudioListener>();

	uvCheckerTexture_ = AssetDatabase::LoadAssetAtPath<Texture>("Assets/uvChecker.png");
	softCircleTexture_ = AssetDatabase::LoadAssetAtPath<Texture>("Assets/Particle/soft_circle.png");

	// エミッタ: 円錐で上に出して、寿命の終わりに向かって薄く・小さくする
	emitter_ = scene_->CreateGameObject("Particle Emitter");
	particleRenderer_ = emitter_->AddComponent<ParticleSystemRenderer>();
	particleRenderer_->GetMaterial()->SetMainTexture(softCircleTexture_); // テクスチャはマテリアルが持つ（Unity と同じ）
	particleSystem_ = emitter_->GetComponent<ParticleSystem>();

	auto &main = particleSystem_->GetMain();
	main.startLifetime = {1.5f, 2.5f};
	main.startSpeed = {2.0f, 4.0f};
	main.startSize = {0.3f, 0.6f};
	main.startColor = ColorRange({1.0f, 0.6f, 0.2f, 1.0f}, {1.0f, 0.9f, 0.4f, 1.0f});
	main.gravityModifier = 0.2f;

	particleSystem_->GetEmission().rateOverTime = 60.0f;

	auto &shape = particleSystem_->GetShape();
	shape.shapeType = ParticleSystem::ShapeType::Cone;
	shape.angle = 20.0f;
	shape.radius = 0.3f;

	particleSystem_->GetColorOverLifetime().enabled = true;
	auto &sizeOverLifetime = particleSystem_->GetSizeOverLifetime();
	sizeOverLifetime.enabled = true;
	sizeOverLifetime.start = 1.0f;
	sizeOverLifetime.end = 0.2f;

	// BGM: ループで、シーンの開始と同時に鳴らす（playOnAwake）
	auto *bgmObject = scene_->CreateGameObject("BGM");
	bgmSource_ = bgmObject->AddComponent<AudioSource>();
	bgmSource_->SetSubmixType(Audio::SubmixType::Music); // clip より先に決めると、インスタンスを作り直さずに済む
	bgmSource_->SetClip(AssetDatabase::LoadAssetAtPath<AudioClip>("Assets/Bgm/maou_14_shining_star.mp3"));
	bgmSource_->SetLoop(true);
	bgmSource_->SetVolume(0.5f);

	// 効果音: ボタンで鳴らす
	seClip_ = AssetDatabase::LoadAssetAtPath<AudioClip>("Assets/Alarm01.wav");
	auto *seObject = scene_->CreateGameObject("SE");
	seSource_ = seObject->AddComponent<AudioSource>();
	seSource_->SetClip(seClip_);
	seSource_->SetPlayOnAwake(false);
}


void GameScene::Update() {
	UpdateEmitterMotion();
	ShowParticleWindow();
	ShowAudioWindow();

	scene_->Update();
}


void ou::GameScene::Draw(Graphics::CommandList cmd) const {
	scene_->Render(cmd);
}


void ou::GameScene::DrawOverlay(Graphics::CommandList cmd) const {
	scene_->RenderOverlay(cmd);
}


void ou::GameScene::UpdateEmitterMotion() {
	auto *transform = emitter_->GetTransform();
	if (!isEmitterMoving_) {
		transform->SetLocalPosition(Vector3::Zero);
		return;
	}

	// 半径 3 の円を 1 周 4 秒で回る
	const auto angle = static_cast<float>(Time::GetTimeAsDouble()) * Mathf::PI * 0.5f;
	transform->SetLocalPosition({Mathf::Cos(angle) * 3.0f, 0.0f, Mathf::Sin(angle) * 3.0f});
}


void ou::GameScene::ShowParticleWindow() {
#ifdef USE_IMGUI
	ImGui::SetNextWindowPos({20.0f, 20.0f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({420.0f, 640.0f}, ImGuiCond_FirstUseEver);
	ImGui::Begin("Particle");

	// 再生
	if (ImGui::Button("再生")) { particleSystem_->Play(); }
	ImGui::SameLine();
	if (ImGui::Button("停止")) { particleSystem_->Stop(); }
	ImGui::SameLine();
	if (ImGui::Button("クリア")) { particleSystem_->Clear(); }
	ImGui::SameLine();
	if (ImGui::Button("30 個出す")) { particleSystem_->Emit(30); }
	ImGui::Text("粒の数: %zu  %s", particleSystem_->GetParticles().size(), particleSystem_->IsPlaying() ? "再生中" : "停止中");
	ImGui::Checkbox("エミッタを円を描くように動かす", &isEmitterMoving_);

	// Main
	if (ImGui::CollapsingHeader("Main", ImGuiTreeNodeFlags_DefaultOpen)) {
		auto &main = particleSystem_->GetMain();
		ImGui::DragFloat("duration", &main.duration, 0.1f, 0.1f, 60.0f);
		ImGui::Checkbox("looping", &main.looping);
		EditFloatRange("startLifetime", main.startLifetime, 0.05f, 0.05f, 20.0f);
		EditFloatRange("startSpeed", main.startSpeed, 0.05f, 0.0f, 50.0f);
		EditFloatRange("startSize", main.startSize, 0.01f, 0.01f, 10.0f);
		EditColorRange("startColor", main.startColor);
		ImGui::DragFloat("gravityModifier", &main.gravityModifier, 0.01f, -5.0f, 5.0f);

		auto maxParticles = static_cast<int>(main.maxParticles);
		if (ImGui::DragInt("maxParticles", &maxParticles, 10.0f, 1, 100000)) {
			main.maxParticles = static_cast<uint32_t>(maxParticles);
		}

		auto space = static_cast<int>(main.simulationSpace);
		if (ImGui::Combo("simulationSpace", &space, "Local\0World\0")) {
			main.simulationSpace = static_cast<ParticleSystem::SimulationSpace>(space);
			particleSystem_->Clear(); // 空間が変わると、今いる粒の座標の意味が変わるので消す
		}
	}

	// Emission
	if (ImGui::CollapsingHeader("Emission", ImGuiTreeNodeFlags_DefaultOpen)) {
		auto &emission = particleSystem_->GetEmission();
		ImGui::Checkbox("enabled##emission", &emission.enabled);
		ImGui::DragFloat("rateOverTime", &emission.rateOverTime, 1.0f, 0.0f, 10000.0f);

		ImGui::Text("bursts");
		for (size_t i = 0; i < emission.bursts.size(); i++) {
			auto &burst = emission.bursts[i];
			ImGui::PushID(static_cast<int>(i));
			ImGui::SetNextItemWidth(120.0f);
			ImGui::DragFloat("time", &burst.time, 0.05f, 0.0f, 60.0f);
			ImGui::SameLine();
			auto count = static_cast<int>(burst.count);
			ImGui::SetNextItemWidth(120.0f);
			if (ImGui::DragInt("count", &count, 1.0f, 0, 10000)) {
				burst.count = static_cast<uint32_t>(count);
			}
			ImGui::SameLine();
			const auto isRemoved = ImGui::Button("削除");
			ImGui::PopID();

			if (isRemoved) {
				emission.bursts.erase(emission.bursts.begin() + static_cast<std::ptrdiff_t>(i));
				break;
			}
		}
		if (ImGui::Button("バーストを追加")) {
			emission.bursts.push_back({});
		}
	}

	// Shape
	if (ImGui::CollapsingHeader("Shape", ImGuiTreeNodeFlags_DefaultOpen)) {
		auto &shape = particleSystem_->GetShape();
		ImGui::Checkbox("enabled##shape", &shape.enabled);

		auto shapeType = static_cast<int>(shape.shapeType);
		if (ImGui::Combo("shapeType", &shapeType, "Sphere\0Cone\0")) {
			shape.shapeType = static_cast<ParticleSystem::ShapeType>(shapeType);
		}
		if (shape.shapeType == ParticleSystem::ShapeType::Cone) {
			ImGui::SliderFloat("angle", &shape.angle, 0.0f, 90.0f, "%.1f 度");
		}
		ImGui::DragFloat("radius", &shape.radius, 0.01f, 0.0f, 20.0f);
		if (shape.shapeType == ParticleSystem::ShapeType::Sphere) {
			ImGui::SliderFloat("radiusThickness", &shape.radiusThickness, 0.0f, 1.0f);
		}
	}

	// Color / Size over Lifetime
	if (ImGui::CollapsingHeader("Color over Lifetime", ImGuiTreeNodeFlags_DefaultOpen)) {
		auto &colorOverLifetime = particleSystem_->GetColorOverLifetime();
		ImGui::Checkbox("enabled##color", &colorOverLifetime.enabled);
		ImGui::ColorEdit4("start##color", &colorOverLifetime.start.r);
		ImGui::ColorEdit4("end##color", &colorOverLifetime.end.r);
	}
	if (ImGui::CollapsingHeader("Size over Lifetime", ImGuiTreeNodeFlags_DefaultOpen)) {
		auto &sizeOverLifetime = particleSystem_->GetSizeOverLifetime();
		ImGui::Checkbox("enabled##size", &sizeOverLifetime.enabled);
		ImGui::DragFloat("start##size", &sizeOverLifetime.start, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("end##size", &sizeOverLifetime.end, 0.01f, 0.0f, 10.0f);
	}

	// Renderer
	if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::Combo("texture", &textureIndex_, "白（なし）\0uvChecker\0丸いぼかし\0")) {
			const std::shared_ptr<Texture> textures[] = {nullptr, uvCheckerTexture_, softCircleTexture_};
			particleRenderer_->GetMaterial()->SetMainTexture(textures[textureIndex_]);
		}

		auto blendMode = static_cast<int>(particleRenderer_->GetBlendMode());
		// 並びは ParticleSystemRenderer::BlendMode と同じ
		if (ImGui::Combo("blendMode", &blendMode, "加算\0減算\0乗算\0アルファブレンド\0")) {
			particleRenderer_->SetBlendMode(static_cast<ParticleSystemRenderer::BlendMode>(blendMode));
		}

		// 並びは ParticleSystemRenderer::SortMode と同じ。アルファブレンドのときは None 以外にする
		auto sortMode = static_cast<int>(particleRenderer_->GetSortMode());
		if (ImGui::Combo("sortMode", &sortMode, "None\0Distance\0OldestInFront\0YoungestInFront\0")) {
			particleRenderer_->SetSortMode(static_cast<ParticleSystemRenderer::SortMode>(sortMode));
		}
	}

	ImGui::End();
#endif
}


void ou::GameScene::ShowAudioWindow() {
#ifdef USE_IMGUI
	ImGui::SetNextWindowPos({460.0f, 20.0f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({400.0f, 500.0f}, ImGuiCond_FirstUseEver);
	ImGui::Begin("Audio");

	// 全体（AudioListener）とサブミックスの音量
	auto masterVolume = AudioListener::GetVolume();
	if (ImGui::SliderFloat("全体の音量", &masterVolume, 0.0f, 1.0f)) {
		AudioListener::SetVolume(masterVolume);
	}
	auto musicVolume = Audio::GetSubmixVolume(Audio::SubmixType::Music);
	if (ImGui::SliderFloat("BGM の音量", &musicVolume, 0.0f, 1.0f)) {
		Audio::SetSubmixVolume(Audio::SubmixType::Music, musicVolume);
	}
	auto soundEffectVolume = Audio::GetSubmixVolume(Audio::SubmixType::SoundEffect);
	if (ImGui::SliderFloat("効果音の音量", &soundEffectVolume, 0.0f, 1.0f)) {
		Audio::SetSubmixVolume(Audio::SubmixType::SoundEffect, soundEffectVolume);
	}

	EditAudioSource("BGM", *bgmSource_);

	if (EditAudioSource("SE", *seSource_)) {
		// clip とは別に重ねて鳴らす。連打すると重なる
		if (ImGui::Button("PlayOneShot")) {
			seSource_->PlayOneShot(seClip_, 1.0f);
		}
		ImGui::SameLine();
		if (ImGui::Button("PlayOneShot（半分の音量）")) {
			seSource_->PlayOneShot(seClip_, 0.5f);
		}
	}

	ImGui::End();
#endif
}
