#pragma once

#include "Scene/GameObject.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>


namespace ou {

class Scene {
	friend class SceneManager;
	friend class SceneSerializer;
	friend class EditorSceneManager;

public: // 静的メンバ関数

	static uint64_t GetDestroyGeneration();


public: // メンバ関数

	Scene(std::string_view name, int32_t buildIndex);
	Scene(const Scene &) = delete;
	Scene &operator=(const Scene &) = delete;
	~Scene();

	GameObject *CreateGameObject(std::string_view name = "GameObject");
	GameObject *Find(std::string_view name) const;
	std::vector<GameObject *> GetRootGameObjects() const;

	const std::string &GetName() const;
	const std::string &GetPath() const;
	int32_t GetBuildIndex() const;
	bool IsDirty() const;


private: // 静的メンバ変数

	static inline uint64_t sDestroyGeneration_ = 0;


private: // メンバ変数

	std::vector<std::unique_ptr<GameObject>> gameObjects_;

	std::string name_;
	std::string path_;
	int32_t buildIndex_ = -1;
	bool isDirty_ = false;


private: // メンバ関数

	void StartGameObjects();
	void FixedUpdateGameObjects();
	void UpdateGameObjects();
	void LateUpdateGameObjects();
	void DestroyMarkedObjects();
	void DestroyAllGameObjects();

	static void MarkDescendantsDestroyed(GameObject *gameObject);
};

} // namespace ou
