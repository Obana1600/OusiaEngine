#pragma once

#include "Scene/Object.h"

#include <memory>
#include <string>
#include <string_view>
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
		std::shared_ptr<Mesh> mesh;
		std::vector<std::shared_ptr<Material>> materials;
	};


public: // 静的メンバ関数

	static std::shared_ptr<Model> LoadObj(std::string_view fileName);


public: // メンバ関数

	GameObject *Instantiate(Scene *scene) const;

	const std::vector<Node> &GetNodes() const;


private: // メンバ変数

	std::vector<Node> nodes_;
};

} // namespace ou
