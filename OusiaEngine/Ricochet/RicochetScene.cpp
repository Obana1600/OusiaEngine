#include "RicochetScene.h"
#include "Arena.h"
#include "Boss.h"
#include "BossWeakPoint.h"
#include "CameraRig.h"
#include "CombatSystem.h"
#include "ComboCounter.h"
#include "GameAssets.h"
#include "GameFlow.h"
#include "Hud.h"
#include "Math/Mathf.h"
#include "Player.h"
#include "PlayerVisual.h"
#include "Render/Light.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "TimeControl.h"
#include "TrajectoryPreview.h"

using namespace ou;



void ou::BuildRicochetScene(Scene &scene) {
	scene.CreateGameObject("Game Assets")->AddComponent<GameAssets>();
	scene.CreateGameObject("Time Control")->AddComponent<TimeControl>();

	auto *light = scene.CreateGameObject("Light");
	light->AddComponent<Light>()->SetType(LightType::Directional);
	light->GetTransform()->SetLocalEulerAngles(Vector3{35.0f, -25.0f, 0.0f} * Mathf::Deg2Rad);

	scene.CreateGameObject("Main Camera")->AddComponent<CameraRig>();
	CreateArena(scene);

	auto *player = scene.CreateGameObject("Player");
	player->AddComponent<Player>();
	player->AddComponent<PlayerVisual>();

	auto *boss = scene.CreateGameObject("Boss");
	boss->AddComponent<Boss>();
	boss->AddComponent<BossWeakPoint>();

	scene.CreateGameObject("Combo Counter")->AddComponent<ComboCounter>();
	scene.CreateGameObject("Combat System")->AddComponent<CombatSystem>();
	scene.CreateGameObject("Trajectory Preview")->AddComponent<TrajectoryPreview>();
	scene.CreateGameObject("Hud")->AddComponent<Hud>();
	scene.CreateGameObject("Game Flow")->AddComponent<GameFlow>();
}
