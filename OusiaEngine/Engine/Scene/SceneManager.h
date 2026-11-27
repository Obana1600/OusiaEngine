#pragma once

#include "Core/Event.h"
#include "Core/JsonFile.h"
#include "Graphics/GraphicsTypes.h"
#include "Render/Camera.h"
#include "Render/RenderQueueRange.h"
#include "Scene/Scene.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>


namespace ou {

// 前方宣言
class GameObject;
class Renderer;
class Light;
class Canvas;
class SkeletalAnimator;
struct PerFrame;

class SceneManager final {
	friend class Object;
	friend class Engine;
	friend class EditorApplication;
	friend class EditorSceneManager;
	friend class Undo;
	friend class EditorBuildSettings;
	friend class ScriptBindings;
	friend class GameObject; // FindWithTag で DontDestroyOnLoad のシーンも探す


public: // 静的メンバ変数

	static inline Event<Scene *> sceneLoaded_;
	static inline Event<Scene *> sceneUnloaded_;
	static inline Event<Scene *, Scene *> activeSceneChanged_;


public: // 静的メンバ関数

	static int32_t GetSceneCountInBuildSettings();

	static void LoadScene(std::string_view name);
	static void LoadScene(int32_t buildIndex);

	static Scene *GetActiveScene();
	static void MoveGameObjectToScene(GameObject *gameObject, Scene *scene);

	static void RegisterCamera(Camera *camera);
	static void UnregisterCamera(Camera *camera);
	static Camera *GetMainCamera();

	static void RegisterRenderer(Renderer *renderer);
	static void UnregisterRenderer(Renderer *renderer);

	static void RegisterLight(Light *light);
	static void UnregisterLight(Light *light);

	static void RegisterCanvas(Canvas *canvas);
	static void UnregisterCanvas(Canvas *canvas);

	static void RegisterSkeletalAnimator(SkeletalAnimator *animator);
	static void UnregisterSkeletalAnimator(SkeletalAnimator *animator);


private: // インナークラス

	struct BuildScene {
		std::string name;
		std::string path;
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
	static inline std::vector<SkeletalAnimator *> sSkeletalAnimators_;


private: // 静的メンバ関数

	static void LoadBuildSettings();

	static void Update();
	static void Finalize();

	static void CollectLights(PerFrame &perFrame);
	static void RenderWithCamera(const Camera::PerCamera &perCamera, const RenderQueueRange &range, Graphics::CommandList cmd);
	static void RenderOverlay(Graphics::CommandList cmd);

	static void LoadPendingScene();

	static Scene *GetDontDestroyOnLoadScene();
	static void MoveHierarchy(GameObject *gameObject, Scene *scene);

	static void NotifyApplicationQuit();
	static void UnloadAllScenes();

	static void ReplaceActiveScene(std::string_view name, std::string_view path, int32_t buildIndex, const Json &json);
	static int32_t FindBuildIndexByPath(std::string_view path);
};

} // namespace ou
