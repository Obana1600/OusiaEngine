#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Scene/GameObject.h"

#include <memory>
#include <string_view>
#include <vector>


namespace ou {

// 前方宣言
class Camera;
class Renderer;
class DirectionalLight;
class Canvas;

class Scene {
public: // メンバ関数

	Scene() = default;
	Scene(const Scene &) = delete;
	Scene &operator=(const Scene &) = delete;

	~Scene();

	GameObject *CreateGameObject(std::string_view name = "GameObject");

	GameObject *Find(std::string_view name) const;

	void Update();
	void Render(Graphics::CommandList cmd) const;
	void RenderOverlay(Graphics::CommandList cmd) const;

	// Camera
	void RegisterCamera(Camera *camera);
	void UnregisterCamera(Camera *camera);
	Camera *GetMainCamera() const;

	// Renderer
	void RegisterRenderer(Renderer *renderer);
	void UnregisterRenderer(Renderer *renderer);

	// Light
	void RegisterLight(DirectionalLight *light);
	void UnregisterLight(DirectionalLight *light);

	// ui
	void RegisterCanvas(Canvas *canvas);
	void UnregisterCanvas(Canvas *canvas);


private: // メンバ変数

	std::vector<Camera *> cameras_;
	std::vector<Renderer *> renderers_;
	std::vector<Canvas *> canvases_;
	std::vector<DirectionalLight *> lights_;
	std::vector<std::unique_ptr<GameObject>> gameObjects_;


private: // メンバ関数

	void DestroyMarkedObjects();

	static void MarkDescendantsDestroyed(GameObject *gameObject);
};

} // namespace ou
