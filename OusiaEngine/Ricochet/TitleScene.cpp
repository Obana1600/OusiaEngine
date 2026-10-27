#include "TitleScene.h"
#include "Arena.h"
#include "CameraRig.h"
#include "GameAssets.h"
#include "RicochetScene.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "StageParticles.h"
#include "TitleMenu.h"

using namespace ou;



void ou::BuildTitleScene(Scene &scene) {
	scene.CreateGameObject("Game Assets")->AddComponent<GameAssets>();
	CreateStageLights(scene);
	scene.CreateGameObject("Main Camera")->AddComponent<CameraRig>();
	CreateArena(scene);
	scene.CreateGameObject("Stage Particles")->AddComponent<StageParticles>();
	scene.CreateGameObject("Title Menu")->AddComponent<TitleMenu>();
}
