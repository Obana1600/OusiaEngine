#pragma once

#include "Core/Object.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"

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
class SkeletalAnimationClip;

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
		std::vector<int> bones;
	};


public: // メンバ関数

	explicit Model(std::vector<Node> nodes);

	GameObject *Instantiate(Scene *scene) const;

	const std::vector<Node> &GetNodes() const;

	void SetAnimationClips(std::vector<std::shared_ptr<SkeletalAnimationClip>> clips);
	const std::vector<std::shared_ptr<SkeletalAnimationClip>> &GetAnimationClips() const;
	std::shared_ptr<SkeletalAnimationClip> FindAnimationClip(std::string_view name) const;


private: // メンバ変数

	std::vector<Node> nodes_;
	std::vector<std::shared_ptr<SkeletalAnimationClip>> animationClips_;
};

} // namespace ou
