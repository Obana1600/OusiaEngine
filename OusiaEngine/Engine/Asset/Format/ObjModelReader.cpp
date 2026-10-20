#include "ObjModelReader.h"
#include "Core/Log.h"
#include "Core/Paths.h"

#include <unordered_map>
#include <utility>

#pragma warning(push, 0)
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>
#pragma warning(pop)

using namespace ou;


namespace {

struct CornerKey {
	int position;
	int texcoord;
	int normal;

	bool operator==(const CornerKey &) const = default;
};

struct CornerKeyHash {
	size_t operator()(const CornerKey &key) const {
		size_t hash = std::hash<int>{}(key.position);
		hash = hash * 31 + std::hash<int>{}(key.texcoord);
		hash = hash * 31 + std::hash<int>{}(key.normal);
		return hash;
	}
};

Mesh::Vertex MakeVertex(const tinyobj::attrib_t &attrib, const tinyobj::index_t &index) {
	Mesh::Vertex vertex{};

	const auto p = 3 * static_cast<size_t>(index.vertex_index);
	vertex.position = {-attrib.vertices[p], attrib.vertices[p + 1], attrib.vertices[p + 2]};

	if (index.normal_index >= 0) {
		const auto n = 3 * static_cast<size_t>(index.normal_index);
		vertex.normal = {-attrib.normals[n], attrib.normals[n + 1], attrib.normals[n + 2]};
	}

	if (index.texcoord_index >= 0) {
		const auto t = 2 * static_cast<size_t>(index.texcoord_index);
		vertex.texcoord = {attrib.texcoords[t], 1.0f - attrib.texcoords[t + 1]};
	}

	return vertex;
}

ModelData::MeshData BuildMesh(const tinyobj::attrib_t &attrib, const tinyobj::shape_t &shape) {
	const auto &mesh = shape.mesh;

	std::vector<int> materialOrder;
	std::unordered_map<int, std::vector<size_t>> facesByMaterial;
	for (size_t face = 0; face < mesh.material_ids.size(); face++) {
		const auto materialId = mesh.material_ids[face];
		auto &faces = facesByMaterial[materialId];
		if (faces.empty()) {
			materialOrder.push_back(materialId);
		}
		faces.push_back(face);
	}

	ModelData::MeshData data{};
	data.name = shape.name;
	std::unordered_map<CornerKey, uint32_t, CornerKeyHash> uniqueVertices;

	for (const auto materialId : materialOrder) {
		Mesh::SubMesh subMesh{};
		subMesh.indexStart = static_cast<uint32_t>(data.indices.size());

		for (const auto face : facesByMaterial[materialId]) {
			const size_t kCornerOrder[] = {0, 2, 1};
			for (const auto corner : kCornerOrder) {
				const auto &index = mesh.indices[3 * face + corner];
				const CornerKey key = {index.vertex_index, index.texcoord_index, index.normal_index};
				const auto [it, inserted] = uniqueVertices.try_emplace(key, static_cast<uint32_t>(data.vertices.size()));
				if (inserted) {
					data.vertices.push_back(MakeVertex(attrib, index));
				}
				data.indices.push_back(it->second);
			}
		}

		subMesh.indexCount = static_cast<uint32_t>(data.indices.size()) - subMesh.indexStart;
		data.submeshes.push_back(subMesh);
		data.materialIndices.push_back(materialId);
	}

	return data;
}

} // namespace



std::optional<ModelData> ou::ReadObjModel(const std::filesystem::path &path) {
	tinyobj::ObjReaderConfig config;
	config.triangulate = true;
	config.vertex_color = false;

	// ファイル読み込み
	tinyobj::ObjReader reader;
	if (!reader.ParseFromFile(path.string(), config)) {
		Log::Error("OBJ を読めませんでした: {}\n{}", ToUtf8(path), reader.Error());
		return std::nullopt;
	}
	if (!reader.Warning().empty()) {
		Log::Warning("OBJ の警告: {}\n{}", ToUtf8(path), reader.Warning());
	}

	ModelData data{};

	// マテリアル
	for (const auto &source : reader.GetMaterials()) {
		ModelData::MaterialData material{};
		material.name = source.name;
		material.mainTexture = source.diffuse_texname;
		material.color = {source.diffuse[0], source.diffuse[1], source.diffuse[2], source.dissolve};
		data.materials.push_back(std::move(material));
	}

	// メッシュ / ノード
	const auto &attrib = reader.GetAttrib();
	for (const auto &shape : reader.GetShapes()) {
		if (shape.mesh.indices.empty()) { continue; }

		ModelData::NodeData node{};
		node.name = shape.name;
		node.mesh = static_cast<int>(data.meshes.size());
		data.meshes.push_back(BuildMesh(attrib, shape));
		data.nodes.push_back(std::move(node));
	}

	return data;
}
