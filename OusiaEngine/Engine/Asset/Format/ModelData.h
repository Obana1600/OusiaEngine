#pragma once

#include "Math/Color.h"
#include "Math/Matrix4x4.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Render/Asset/Mesh.h"

#include <cstdint>
#include <string>
#include <vector>


namespace ou {

struct ModelData {
	struct MeshData {
		std::string name;
		std::vector<Mesh::Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<Mesh::SubMesh> submeshes;
		std::vector<int> materialIndices;
		std::vector<Mesh::BoneWeight> boneWeights;
	};

	struct MaterialData {
		std::string name;
		std::string mainTexture;
		Color color = Color::White;
	};

	struct NodeData {
		std::string name;
		Vector3 position = Vector3::Zero;
		Quaternion rotation = Quaternion::Identity;
		Vector3 scale = Vector3::One;
		int parent = -1;
		int mesh = -1;
		int skin = -1;
	};

	struct SkinData {
		std::vector<int> joints;
		std::vector<Matrix4x4> inverseBindMatrices;
	};

	struct AnimationChannelData {
		enum class Path {
			Translation,
			Rotation,
			Scale,
		};

		enum class Interpolation {
			Step,
			Linear,
			CubicHermite,
		};

		int node = -1;
		Path path = Path::Translation;
		Interpolation interpolation = Interpolation::Linear;

		std::vector<float> times;
		std::vector<float> values;
	};

	struct AnimationData {
		std::string name;
		std::vector<AnimationChannelData> channels;
	};

	std::vector<MeshData> meshes;
	std::vector<MaterialData> materials;
	std::vector<NodeData> nodes;
	std::vector<SkinData> skins;
	std::vector<AnimationData> animations;
};

} // namespace ou
