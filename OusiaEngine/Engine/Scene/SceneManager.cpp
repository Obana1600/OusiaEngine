#include "SceneManager.h"
#include "Scene/TagManager.h"
#include "Core/JsonFile.h"
#include "Animation/Skeletal/SkeletalAnimator.h"
#include "Animation/Tween/TweenManager.h"
#include "Asset/AssetDatabase.h"
#include "Core/Application.h"
#include "Core/Guid.h"
#include "Core/StringUtil.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Graphics/GraphicsDevice.h"
#include "Physics/Physics.h"
#include "Platform/Paths.h"
#include "Render/Camera.h"
#include "Render/Light.h"
#include "Render/Renderer.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"
#include "Serialize/SceneSerializer.h"
#include "UI/Canvas.h"
#include "UI/CanvasUpdateRegistry.h"
#include "UI/EventSystem/EventSystem.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>

using namespace ou;


namespace {

std::string MakeSceneName(std::string_view scenePath) {
	return WideToUtf8(std::filesystem::path(Utf8ToWide(scenePath)).stem().wstring());
}

} // namespace



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

	std::erase(gameObject->scene_->rootGameObjects_, gameObject);
	scene->rootGameObjects_.push_back(gameObject);

	MoveHierarchy(gameObject, scene);
}


void ou::SceneManager::RegisterCamera(Camera *camera) {
	sCameras_.push_back(camera);
}


void ou::SceneManager::UnregisterCamera(Camera *camera) {
	std::erase(sCameras_, camera);
}


Camera *ou::SceneManager::GetMainCamera() {
	// Unity と同じく、MainCamera のタグのカメラを返す
	// （タグを付けていなかったころのシーンも映るように、なければ、動いている最初のカメラ）
	Camera *firstEnabled = nullptr;
	for (auto *camera : sCameras_) {
		if (!camera->IsActiveAndEnabled()) { continue; }
		if (camera->GetGameObject()->GetTag() == TagManager::kMainCamera) { return camera; }
		if (!firstEnabled) { firstEnabled = camera; }
	}
	return firstEnabled;
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


void ou::SceneManager::RegisterSkeletalAnimator(SkeletalAnimator *animator) {
	sSkeletalAnimators_.push_back(animator);
}


void ou::SceneManager::UnregisterSkeletalAnimator(SkeletalAnimator *animator) {
	std::erase(sSkeletalAnimators_, animator);
}


void ou::SceneManager::LoadBuildSettings() {
	sBuildScenes_.clear();

	const auto folder = FindProjectSettingsFolder();
	if (!folder) { return; }
	const auto path = *folder / L"EditorBuildSettings.json";
	if (!std::filesystem::exists(path)) { return; }

	const auto json = JsonFile::Load(path);
	if (!json) { return; }

	try {
		const auto it = json->find("scenes");
		if (it == json->end() || !it->is_array()) { return; }

		for (const auto &entry : *it) {
			if (!entry.value("enabled", true)) { continue; }

			// guid を先に見る
			auto scenePath = entry.value("path", std::string{});
			if (const auto guid = Guid::Parse(entry.value("guid", std::string{}))) {
				if (auto found = AssetDatabase::GuidToAssetPath(*guid); !found.empty()) {
					scenePath = std::move(found);
				}
			}
			if (scenePath.empty()) { continue; }

			sBuildScenes_.push_back({MakeSceneName(scenePath), scenePath});
		}
	} catch (const Json::exception &e) {
		Log::Error("EditorBuildSettings.json の読み込みに失敗しました: {}\n{}", ToUtf8(path), e.what());
	}
}


void ou::SceneManager::Update() {
	if (sPendingBuildIndex_ >= 0) {
		LoadPendingScene();
	}
	// Play していないとき・Pause で止めているときは、ゲームを動かさない（Inspector で消したものだけ片付ける）
	if (!Application::IsPlaying() || Application::IsPaused()) {
		for (auto *scene : {sDontDestroyOnLoadScene_.get(), sActiveScene_.get()}) {
			if (scene) { scene->DestroyMarkedObjects(); }
		}
		return;
	}

	Scene *const scenes[] = {sDontDestroyOnLoadScene_.get(), sActiveScene_.get()};

	for (auto *scene : scenes) {
		if (scene) { scene->StartGameObjects(); }
	}

	// 物理の更新
	while (Time::AdvanceFixedTime()) {
		for (auto *scene : scenes) {
			if (scene) { scene->FixedUpdateGameObjects(); }
		}
		Physics::Simulate();
	}
	Physics::Interpolate();

	if (auto *eventSystem = EventSystem::GetCurrent()) {
		eventSystem->Process();
	}

	for (auto *scene : scenes) {
		if (scene) { scene->UpdateGameObjects(); }
	}

	// Tween の更新
	TweenManager::Update(Time::GetDeltaTime(), Time::GetUnscaledDeltaTime());

	// アニメーションの更新
	const auto deltaTime = Time::GetDeltaTime();
	for (auto *animator : sSkeletalAnimators_) {
		if (animator->IsActiveAndEnabled()) { animator->UpdateAnimation(deltaTime); }
	}

	for (auto *scene : scenes) {
		if (scene) { scene->LateUpdateGameObjects(); }
	}
	for (auto *scene : scenes) {
		if (scene) { scene->DestroyMarkedObjects(); }
	}
}


void ou::SceneManager::Finalize() {
	TweenManager::Finalize();

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

	RenderWithCamera(camera->MakePerCamera(), cmd);
}


void ou::SceneManager::RenderWithCamera(const Camera::PerCamera &perCamera, Graphics::CommandList cmd) {
	auto *device = Graphics::GetDevice();
	device->BindDynamicConstantBuffer(perCamera, 2, cmd);

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
	CanvasUpdateRegistry::PerformUpdate();

	for (const auto *canvas : sCanvases_) {
		if (canvas->IsActiveAndEnabled()) {
			canvas->Render(cmd);
		}
	}
}


void ou::SceneManager::LoadPendingScene() {
	const auto buildIndex = std::exchange(sPendingBuildIndex_, -1);

	if (auto oldScene = std::move(sActiveScene_)) {
		oldScene->DestroyAllGameObjects();
		sceneUnloaded_.Invoke(oldScene.get());
	}

	const auto &buildScene = sBuildScenes_[buildIndex];
	sActiveScene_ = std::make_unique<Scene>(buildScene.name, buildIndex);
	sActiveScene_->path_ = buildScene.path;

	if (const auto fullPath = AssetDatabase::AssetPathToFullPath(buildScene.path)) {
		SceneSerializer::Load(*sActiveScene_, *fullPath);
	} else {
		Log::Error("シーンのパスは Assets/ か EngineAssets/ で始めてください: {}", buildScene.path);
	}

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


void ou::SceneManager::ReplaceActiveScene(std::string_view name, std::string_view path, int32_t buildIndex, const Json &json) {
	sPendingBuildIndex_ = -1;

	if (sDontDestroyOnLoadScene_) {
		sDontDestroyOnLoadScene_->DestroyAllGameObjects();
	}

	if (auto oldScene = std::move(sActiveScene_)) {
		oldScene->DestroyAllGameObjects();
		sceneUnloaded_.Invoke(oldScene.get());
	}

	sActiveScene_ = std::make_unique<Scene>(name, buildIndex);
	sActiveScene_->path_ = path;
	SceneSerializer::FromJson(*sActiveScene_, json);

	activeSceneChanged_.Invoke(nullptr, sActiveScene_.get());
	sceneLoaded_.Invoke(sActiveScene_.get());
}


int32_t ou::SceneManager::FindBuildIndexByPath(std::string_view path) {
	for (size_t i = 0; i < sBuildScenes_.size(); i++) {
		if (!sBuildScenes_[i].path.empty() && sBuildScenes_[i].path == path) {
			return static_cast<int32_t>(i);
		}
	}
	return -1;
}
