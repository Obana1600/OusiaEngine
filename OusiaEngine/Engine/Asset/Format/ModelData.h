#pragma once

#include "Math/Color.h"
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
	};

	std::vector<NodeData> nodes;
	std::vector<MeshData> meshes;
	std::vector<MaterialData> materials;
};

} // namespace ou
