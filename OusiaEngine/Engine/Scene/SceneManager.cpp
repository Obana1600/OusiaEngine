#include "SceneManager.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Graphics/GraphicsDevice.h"
#include "Physics/Physics.h"
#include "Render/Camera.h"
#include "Render/Light.h"
#include "Render/Renderer.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"
#include "UI/Canvas.h"

#include <algorithm>
#include <utility>

using namespace ou;


void ou::SceneManager::AddScene(std::string_view name, SceneBuilder builder) {
	sBuildScenes_.push_back({std::string(name), std::move(builder)});
}


int32_t ou::SceneManager::GetSceneCountInBuildSettings() {
	return static_cast<int32_t>(sBuildScenes_.size());
}


void ou::SceneManager::LoadScene(std::string_view name) {
	for (size_t i = 0; i < sBuildScenes_.size(); i++) {
		if (sBuildScenes_[i].name == name) {
			sPendingBuildIndex_ = static_cast<int32_t>(i);
			return;
		}
	}

	Log::Error("シーン \"{}\" は登録されていません", name);
}


void ou::SceneManager::LoadScene(int32_t buildIndex) {
	if (buildIndex < 0 || buildIndex >= GetSceneCountInBuildSettings()) {
		Log::Error("build index {} のシーンは登録されていません", buildIndex);
		return;
	}

	sPendingBuildIndex_ = buildIndex;
}


Scene *ou::SceneManager::GetActiveScene() {
	return sActiveScene_.get();
}


void ou::SceneManager::MoveGameObjectToScene(GameObject *gameObject, Scene *scene) {
	if (gameObject->GetTransform()->GetParent()) {
		Log::Error("\"{}\" はルートの GameObject ではないため、別のシーンに移せません", gameObject->GetName());
		return;
	}
	if (gameObject->GetScene() == scene) { return; }

	MoveHierarchy(gameObject, scene);
}


void ou::SceneManager::Update() {
	if (sPendingBuildIndex_ >= 0) {
		LoadPendingScene();
	}

	Scene *const scenes[] = {sDontDestroyOnLoadScene_.get(), sActiveScene_.get()};

	for (auto *scene : scenes) {
		if (scene) { scene->StartGameObjects(); }
	}

	while (Time::AdvanceFixedTime()) {
		for (auto *scene : scenes) {
			if (scene) { scene->FixedUpdateGameObjects(); }
		}
		Physics::Simulate();
	}

	for (auto *scene : scenes) {
		if (scene) { scene->UpdateGameObjects(); }
	}
	for (auto *scene : scenes) {
		if (scene) { scene->LateUpdateGameObjects(); }
	}
	for (auto *scene : scenes) {
		if (scene) { scene->DestroyMarkedObjects(); }
	}
}


void ou::SceneManager::Finalize() {
	sActiveScene_.reset();
	sDontDestroyOnLoadScene_.reset();
	sBuildScenes_.clear();
	sPendingBuildIndex_ = -1;

	sceneLoaded_.RemoveAllListeners();
	sceneUnloaded_.RemoveAllListeners();
	activeSceneChanged_.RemoveAllListeners();
}


void ou::SceneManager::Render(Graphics::CommandList cmd) {
	const auto *camera = GetMainCamera();
	if (!camera) { return; }

	auto *device = Graphics::GetDevice();
	device->BindDynamicConstantBuffer(camera->MakePerCamera(), 2, cmd);

	// ライトのセット
	Light::PerFrame perFrame{};
	for (const auto *light : sLights_) {
		if (!light->IsActiveAndEnabled()) { continue; }

		if (perFrame.lightCount >= Light::kMaxLights) {
			static bool warned = false;
			if (!warned) {
				Log::Warning("有効なライトが {} を超えたため、超過分は適用されません", Light::kMaxLights);
				warned = true;
			}
			break;
		}

		perFrame.lights[perFrame.lightCount++] = light->MakeShaderLight();
	}
	device->BindDynamicConstantBuffer(perFrame, 3, cmd);

	// renderQueue の小さい順に描く
	auto renderers = sRenderers_;
	std::erase_if(renderers, [](const auto *r) {
		return !r->IsActiveAndEnabled();
	});
	std::stable_sort(renderers.begin(), renderers.end(), [](const auto *a, const auto *b) {
		return a->GetRenderQueue() < b->GetRenderQueue();
	});
	for (const auto *renderer : renderers) {
		renderer->Draw(cmd);
	}
}


void ou::SceneManager::RenderOverlay(Graphics::CommandList cmd) {
	for (const auto *canvas : sCanvases_) {
		if (canvas->IsActiveAndEnabled()) {
			canvas->Render(cmd);
		}
	}
}


void ou::SceneManager::RegisterCamera(Camera *camera) {
	sCameras_.push_back(camera);
}


void ou::SceneManager::UnregisterCamera(Camera *camera) {
	std::erase(sCameras_, camera);
}


Camera *ou::SceneManager::GetMainCamera() {
	for (auto *camera : sCameras_) {
		if (camera->IsActiveAndEnabled()) { return camera; }
	}
	return nullptr;
}


void ou::SceneManager::RegisterRenderer(Renderer *renderer) {
	sRenderers_.push_back(renderer);
}


void ou::SceneManager::UnregisterRenderer(Renderer *renderer) {
	std::erase(sRenderers_, renderer);
}


void ou::SceneManager::RegisterLight(Light *light) {
	sLights_.push_back(light);
}


void ou::SceneManager::UnregisterLight(Light *light) {
	std::erase(sLights_, light);
}


void ou::SceneManager::RegisterCanvas(Canvas *canvas) {
	sCanvases_.push_back(canvas);
}


void ou::SceneManager::UnregisterCanvas(Canvas *canvas) {
	std::erase(sCanvases_, canvas);
}


void ou::SceneManager::LoadPendingScene() {
	const auto buildIndex = std::exchange(sPendingBuildIndex_, -1);

	if (auto oldScene = std::move(sActiveScene_)) {
		oldScene->DestroyAllGameObjects();
		sceneUnloaded_.Invoke(oldScene.get());
	}

	const auto &buildScene = sBuildScenes_[buildIndex];
	sActiveScene_ = std::make_unique<Scene>(buildScene.name, buildIndex);
	buildScene.builder(*sActiveScene_);

	activeSceneChanged_.Invoke(nullptr, sActiveScene_.get());
	sceneLoaded_.Invoke(sActiveScene_.get());
}


Scene *ou::SceneManager::GetDontDestroyOnLoadScene() {
	if (!sDontDestroyOnLoadScene_) {
		sDontDestroyOnLoadScene_ = std::make_unique<Scene>("DontDestroyOnLoad", -1);
	}
	return sDontDestroyOnLoadScene_.get();
}


void ou::SceneManager::MoveHierarchy(GameObject *gameObject, Scene *scene) {
	auto &from = gameObject->scene_->gameObjects_;
	auto it = std::find_if(from.begin(), from.end(), [gameObject](const auto &obj) {
		return obj.get() == gameObject;
	});

	scene->gameObjects_.push_back(std::move(*it));
	gameObject->scene_ = scene;

	for (auto *child : gameObject->GetTransform()->GetChildren()) {
		MoveHierarchy(child->GetGameObject(), scene);
	}
}
