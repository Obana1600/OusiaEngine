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

public: // メンバ関数

	Scene(std::string_view name, int32_t buildIndex);
	Scene(const Scene &) = delete;
	Scene &operator=(const Scene &) = delete;

	~Scene();

	GameObject *CreateGameObject(std::string_view name = "GameObject");
	GameObject *Find(std::string_view name) const;

	const std::string &GetName() const;
	int32_t GetBuildIndex() const;


private: // メンバ変数

	std::vector<std::unique_ptr<GameObject>> gameObjects_;

	std::string name_;
	int32_t buildIndex_ = -1;


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
