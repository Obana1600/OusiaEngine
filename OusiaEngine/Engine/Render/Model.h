#pragma once

#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Scene/Object.h"

#include <memory>
#include <string>
#include <vector>


namespace ou {

// 前方宣言
class GameObject;
class Material;
class Mesh;
class Scene;

class Model final: public Object {
public: // インナークラス

	struct Node {
		std::string name;
		Vector3 position = Vector3::Zero;
		Quaternion rotation = Quaternion::Identity;
		Vector3 scale = Vector3::One;
		int parent = -1;
		std::shared_ptr<Mesh> mesh;
		std::vector<std::shared_ptr<Material>> materials;
	};


public: // メンバ関数

	explicit Model(std::vector<Node> nodes);

	GameObject *Instantiate(Scene *scene) const;

	const std::vector<Node> &GetNodes() const;


private: // メンバ変数

	std::vector<Node> nodes_;
};

} // namespace ou
