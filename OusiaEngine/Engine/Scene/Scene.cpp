#include "Scene.h"
#include "Graphics/GraphicsDevice.h"
#include "Render/Material.h"
#include "Render/Renderer.h"
#include "Scene/Camera.h"
#include "Scene/Component.h"
#include "Scene/DirectionalLight.h"
#include "Scene/Transform.h"
#include "UI/Canvas.h"
#include <algorithm>

using namespace ou;


ou::Scene::~Scene() {
	for (const auto &obj : gameObjects_) {
		obj->NotifyDestroyToAllComponents();
	}
	gameObjects_.clear();
}


GameObject *ou::Scene::CreateGameObject(std::string_view name) {
	auto obj = std::make_unique<GameObject>(this, name);
	gameObjects_.push_back(std::move(obj));
	return gameObjects_.back().get();
}


GameObject *ou::Scene::Find(std::string_view name) const {
	for (const auto &obj : gameObjects_) {
		if (!obj->IsDestroyed() && obj->GetName() == name) {
			return obj.get();
		}
	}

	return nullptr;
}


void ou::Scene::Update() {
	const auto count = gameObjects_.size();

	for (size_t i = 0; i < count; i++) {
		if (!gameObjects_[i]->IsDestroyed()) {
			gameObjects_[i]->StartComponents();
		}
	}
	for (size_t i = 0; i < count; i++) {
		if (!gameObjects_[i]->IsDestroyed()) {
			gameObjects_[i]->UpdateComponents();
		}
	}
	for (size_t i = 0; i < count; i++) {
		if (!gameObjects_[i]->IsDestroyed()) {
			gameObjects_[i]->LateUpdateComponents();
		}
	}

	DestroyMarkedObjects();
}


void ou::Scene::Render(Graphics::CommandList cmd) const {
	const auto *camera = GetMainCamera();
	if (!camera) { return; }

	auto *device = Graphics::GetDevice();
	device->BindDynamicConstantBuffer(camera->MakePerCamera(), 2, cmd);

	// ライトのセット
	DirectionalLight::PerFrame perFrame{};
	perFrame.direction = Vector3::Down;
	if (!lights_.empty()) {
		perFrame = lights_.front()->MakePerFrame();
	}
	device->BindDynamicConstantBuffer(perFrame, 3, cmd);

	// renderQueue の小さい順に描く
	auto renderers = renderers_;
	std::stable_sort(renderers.begin(), renderers.end(), [](const auto *a, const auto *b) {
		return a->GetRenderQueue() < b->GetRenderQueue();
	});
	for (const auto *renderer : renderers) {
		renderer->Draw(cmd);
	}
}


void ou::Scene::RenderOverlay(Graphics::CommandList cmd) const {
	for (const auto *canvas : canvases_) {
		canvas->Render(cmd);
	}
}


void ou::Scene::RegisterCamera(Camera *camera) {
	cameras_.push_back(camera);
}


void ou::Scene::UnregisterCamera(Camera *camera) {
	std::erase(cameras_, camera);
}


Camera *ou::Scene::GetMainCamera() const {
	return cameras_.empty() ? nullptr : cameras_.front();
}


void ou::Scene::RegisterRenderer(Renderer *renderer) {
	renderers_.push_back(renderer);
}


void ou::Scene::UnregisterRenderer(Renderer *renderer) {
	std::erase(renderers_, renderer);
}


void ou::Scene::RegisterLight(DirectionalLight *light) {
	lights_.push_back(light);
}


void ou::Scene::UnregisterLight(DirectionalLight *light) {
	std::erase(lights_, light);
}


void ou::Scene::RegisterCanvas(Canvas *canvas) {
	canvases_.push_back(canvas);
}


void ou::Scene::UnregisterCanvas(Canvas *canvas) {
	std::erase(canvases_, canvas);
}


void ou::Scene::DestroyMarkedObjects() {
	for (const auto &obj : gameObjects_) {
		if (obj->IsDestroyed()) {
			MarkDescendantsDestroyed(obj.get());
		}
	}

	for (const auto &obj : gameObjects_) {
		if (obj->IsDestroyed()) {
			obj->NotifyDestroyToAllComponents();
		} else {
			obj->DestroyMarkedComponents();
		}
	}

	// 実際に消す
	std::erase_if(gameObjects_, [](const auto &obj) {
		return obj->IsDestroyed();
	});
}


void ou::Scene::MarkDescendantsDestroyed(GameObject *gameObject) {
	for (auto *child : gameObject->GetTransform()->GetChildren()) {
		Object::Destroy(child->GetGameObject());
		MarkDescendantsDestroyed(child->GetGameObject());
	}
}
