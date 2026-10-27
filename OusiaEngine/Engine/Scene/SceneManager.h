#pragma once

#include "Core/Event.h"
#include "Graphics/GraphicsTypes.h"
#include "Scene/Scene.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>


namespace ou {

// 前方宣言
class GameObject;
class Camera;
class Renderer;
class Light;
class Canvas;

class SceneManager final {
	friend class Object;

public: // エイリアス

	using SceneBuilder = std::function<void(Scene &)>;


public: // 静的メンバ変数

	static inline Event<Scene *> sceneLoaded_;
	static inline Event<Scene *> sceneUnloaded_;
	static inline Event<Scene *, Scene *> activeSceneChanged_;


public: // 静的メンバ関数

	static void AddScene(std::string_view name, SceneBuilder builder);
	static int32_t GetSceneCountInBuildSettings();

	static void LoadScene(std::string_view name);
	static void LoadScene(int32_t buildIndex);

	static Scene *GetActiveScene();
	static void MoveGameObjectToScene(GameObject *gameObject, Scene *scene);

	static void Update();
	static void Finalize();

	static void Render(Graphics::CommandList cmd);
	static void RenderOverlay(Graphics::CommandList cmd);

	static void RegisterCamera(Camera *camera);
	static void UnregisterCamera(Camera *camera);
	static Camera *GetMainCamera();

	static void RegisterRenderer(Renderer *renderer);
	static void UnregisterRenderer(Renderer *renderer);

	static void RegisterLight(Light *light);
	static void UnregisterLight(Light *light);

	static void RegisterCanvas(Canvas *canvas);
	static void UnregisterCanvas(Canvas *canvas);


private: // インナークラス

	struct BuildScene {
		std::string name;
		SceneBuilder builder;
	};


private: // 静的メンバ変数

	static inline std::unique_ptr<Scene> sActiveScene_;
	static inline std::unique_ptr<Scene> sDontDestroyOnLoadScene_;
	static inline std::vector<BuildScene> sBuildScenes_;
	static inline int32_t sPendingBuildIndex_ = -1;

	static inline std::vector<Camera *> sCameras_;
	static inline std::vector<Renderer *> sRenderers_;
	static inline std::vector<Light *> sLights_;
	static inline std::vector<Canvas *> sCanvases_;


private: // 静的メンバ関数

	static void LoadPendingScene();

	static Scene *GetDontDestroyOnLoadScene();
	static void MoveHierarchy(GameObject *gameObject, Scene *scene);
};

} // namespace ou
