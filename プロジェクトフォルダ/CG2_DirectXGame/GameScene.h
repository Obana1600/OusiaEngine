#pragma once

#include "2d/Sprite.h"
#include "3d/Camera.h"
#include "3d/DebugCamera.h"
#include "3d/WorldTransform.h"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Model;


enum class ObjectType {
	Plane,
	Sprite,
	Sphere,
	UtahTeapot,
	StanfordBunny,
	MultiMesh,
	MultiMaterial,
	Suzanne
};


class GameScene {
public: // メンバ関数

	void Initialize();

	void Update();

	void Draw() const;


private: // メンバ変数

	// camera
	DebugCamera camera_{};

	// model
	struct Object3D {
		std::unique_ptr<Model> model;
		WorldTransform worldTransform;
	};
	std::unordered_map<std::string, Object3D> objects_;
	uint32_t objCounter_ = 0;

	// sprite
	std::vector<std::unique_ptr<Sprite>> sprites_;
	uint32_t uvCheckerHandle = 0;


private: // メンバ関数

	void OnImGui();

	void CreateObject(ObjectType type);
};