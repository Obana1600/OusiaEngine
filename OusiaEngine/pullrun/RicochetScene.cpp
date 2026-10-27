#include "RicochetScene.h"
#include "Arena.h"
#include "Boss.h"
#include "BossParticles.h"
#include "BossWeakPoint.h"
#include "CameraRig.h"
#include "CombatSystem.h"
#include "ComboCounter.h"
#include "GameAssets.h"
#include "GameAudio.h"
#include "SkyDome.h"
#include "GameFlow.h"
#include "Hud.h"
#include "Math/Mathf.h"
#include "ParticleEffects.h"
#include "Player.h"
#include "PlayerParticles.h"
#include "PlayerVisual.h"
#include "Render/Light.h"
#include "ResultMenu.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "StageParticles.h"
#include "TimeControl.h"
#include "TrajectoryPreview.h"

using namespace ou;



void ou::CreateStageLights(Scene &scene) {
	// 平行光は控えめにして、プレイヤー・ボス・弾の点光源を目立たせる
	auto *keyLight = scene.CreateGameObject("Key Light");
	auto *key = keyLight->AddComponent<Light>();
	key->SetType(LightType::Directional);
	key->SetColor({1.0f, 0.95f, 0.9f, 1.0f});
	key->SetIntensity(0.6f);
	keyLight->GetTransform()->SetLocalEulerAngles(Vector3{35.0f, -25.0f, 0.0f} * Mathf::Deg2Rad);

	// 反対側からの青みのある弱い光。光の当たらない面が真っ黒にならないようにする
	auto *fillLight = scene.CreateGameObject("Fill Light");
	auto *fill = fillLight->AddComponent<Light>();
	fill->SetType(LightType::Directional);
	fill->SetColor({0.45f, 0.55f, 1.0f, 1.0f});
	fill->SetIntensity(0.25f);
	fillLight->GetTransform()->SetLocalEulerAngles(Vector3{-30.0f, 150.0f, 0.0f} * Mathf::Deg2Rad);
}


void ou::BuildRicochetScene(Scene &scene) {
	scene.CreateGameObject("Game Assets")->AddComponent<GameAssets>();
	scene.CreateGameObject("Time Control")->AddComponent<TimeControl>();
	scene.CreateGameObject("Particle Effects")->AddComponent<ParticleEffects>();
	CreateStageLights(scene);
	GameAudio::EnsureCreated(scene);
	scene.CreateGameObject("Sky Dome")->AddComponent<SkyDome>();

	scene.CreateGameObject("Main Camera")->AddComponent<CameraRig>();
	CreateArena(scene);

	auto *player = scene.CreateGameObject("Player");
	player->AddComponent<Player>();
	player->AddComponent<PlayerVisual>();

	auto *boss = scene.CreateGameObject("Boss");
	boss->AddComponent<Boss>();
	boss->AddComponent<BossWeakPoint>();

	// 付いて回るパーティクル（伸びや回転の影響を受けないよう、別の GameObject にする）
	scene.CreateGameObject("Player Particles")->AddComponent<PlayerParticles>();
	scene.CreateGameObject("Boss Particles")->AddComponent<BossParticles>();
	scene.CreateGameObject("Stage Particles")->AddComponent<StageParticles>();

	scene.CreateGameObject("Combo Counter")->AddComponent<ComboCounter>();
	scene.CreateGameObject("Combat System")->AddComponent<CombatSystem>();
	scene.CreateGameObject("Trajectory Preview")->AddComponent<TrajectoryPreview>();
	scene.CreateGameObject("Hud")->AddComponent<Hud>();
	scene.CreateGameObject("Result Menu")->AddComponent<ResultMenu>();
	scene.CreateGameObject("Game Flow")->AddComponent<GameFlow>();
}
