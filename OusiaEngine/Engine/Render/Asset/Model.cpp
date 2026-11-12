#include "Model.h"
#include "Animation/Skeletal/SkeletalAnimationClip.h"
#include "Animation/Skeletal/SkeletalAnimator.h"
#include "Render/MeshRenderer.h"
#include "Render/SkinnedMeshRenderer.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"

#include <string>
#include <utility>

using namespace ou;


namespace {

std::string MakeNodePath(const std::vector<Model::Node> &nodes, int index) {
	auto path = nodes[index].name;
	for (auto parent = nodes[index].parent; parent >= 0; parent = nodes[parent].parent) {
		path = nodes[parent].name + "/" + path;
	}
	return path;
}

} // namespace



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
		const auto setUpRenderer = [&](auto *renderer) {
			renderer->SetMesh(node.mesh);
			renderer->SetSharedMaterials(node.materials);
		};
		if (node.mesh && !node.bones.empty()) {
			setUpRenderer(object->AddComponent<SkinnedMeshRenderer>());
		} else if (node.mesh) {
			setUpRenderer(object->AddComponent<MeshRenderer>());
		}

		transforms.push_back(transform);
	}

	for (size_t i = 0; i < nodes_.size(); i++) {
		if (!nodes_[i].mesh || nodes_[i].bones.empty()) { continue; }

		std::vector<std::string> bonePaths;
		bonePaths.reserve(nodes_[i].bones.size());
		for (const auto bone : nodes_[i].bones) {
			bonePaths.push_back(MakeNodePath(nodes_, bone));
		}

		auto *renderer = transforms[i]->GetGameObject()->GetComponent<SkinnedMeshRenderer>();
		renderer->SetBones(root->GetTransform(), std::move(bonePaths));
	}

	if (!animationClips_.empty()) {
		root->AddComponent<SkeletalAnimator>();
	}
	return root;
}


const std::vector<Model::Node> &ou::Model::GetNodes() const {
	return nodes_;
}


void ou::Model::SetAnimationClips(std::vector<std::shared_ptr<SkeletalAnimationClip>> clips) {
	animationClips_ = std::move(clips);
}


const std::vector<std::shared_ptr<SkeletalAnimationClip>> &ou::Model::GetAnimationClips() const {
	return animationClips_;
}


std::shared_ptr<SkeletalAnimationClip> ou::Model::FindAnimationClip(std::string_view name) const {
	for (const auto &clip : animationClips_) {
		if (clip->GetName() == name) { return clip; }
	}
	return nullptr;
}
