#include "Model.h"
#include "Render/MeshRenderer.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"

#include <utility>

using namespace ou;


ou::Model::Model(std::vector<Node> nodes)
	: nodes_(std::move(nodes)) {}


GameObject *ou::Model::Instantiate(Scene *scene) const {
	auto *root = scene->CreateGameObject(GetName());

	std::vector<Transform *> transforms;
	transforms.reserve(nodes_.size());
	for (const auto &node : nodes_) {
		auto *object = scene->CreateGameObject(node.name);
		auto *transform = object->GetTransform();

		// トランスフォーム
		transform->SetParent(node.parent >= 0 ? transforms[node.parent] : root->GetTransform());
		transform->SetLocalPosition(node.position);
		transform->SetLocalRotation(node.rotation);
		transform->SetLocalScale(node.scale);

		// メッシュレンダラ
		if (node.mesh) {
			auto *renderer = object->AddComponent<MeshRenderer>();
			renderer->SetMesh(node.mesh);
			renderer->SetSharedMaterials(node.materials);
		}

		transforms.push_back(transform);
	}

	return root;
}


const std::vector<Model::Node> &ou::Model::GetNodes() const {
	return nodes_;
}