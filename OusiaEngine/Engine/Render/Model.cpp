#include "Model.h"
#include "Asset/AssetDatabase.h"
#include "Asset/Material.h"
#include "Asset/Mesh.h"
#include "Core/Log.h"
#include "Core/Paths.h"
#include "Math/Color.h"
#include "Render/MeshRenderer.h"
#include "Render/ObjParser.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"

#include <unordered_map>

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

Mesh::Vertex MakeVertex(const ObjData &data, const ObjData::Corner &corner) {
	Mesh::Vertex vertex{};

	vertex.position = data.positions[corner.position];
	vertex.position.x *= -1.0f;

	if (corner.normal >= 0) {
		vertex.normal = data.normals[corner.normal];
		vertex.normal.x *= -1.0f;
	}

	if (corner.texcoord >= 0) {
		const auto &uv = data.texcoords[corner.texcoord];
		vertex.texcoord = {uv.x, 1.0f - uv.y};
	}

	return vertex;
}

std::vector<std::shared_ptr<Material>> CreateMaterials(const ObjData &data, std::string_view directory) {
	std::vector<std::shared_ptr<Material>> materials;

	for (const auto &source : data.materials) {
		auto material = Material::Create();
		material->SetName(source.name);
		material->SetColor({source.diffuse, source.alpha});
		if (!source.diffuseMap.empty()) {
			material->SetMainTexture(
				AssetDatabase::LoadAssetAtPath<Texture>(
					"Assets/" + std::string(directory) + source.diffuseMap));
		}
		materials.push_back(std::move(material));
	}

	return materials;
}

Model::Node BuildNode(
	const ObjData &data,
	const ObjData::Shape &shape,
	const std::vector<std::shared_ptr<Material>> &materials,
	const std::shared_ptr<Material> &fallbackMaterial) {
	std::vector<int> materialOrder;
	std::unordered_map<int, std::vector<const ObjData::Triangle *>> triangleByMaterial;
	for (const auto &triangle : shape.triangles) {
		auto &list = triangleByMaterial[triangle.material];
		if (list.empty()) {
			materialOrder.push_back(triangle.material);
		}
		list.push_back(&triangle);
	}

	Model::Node node;
	node.name = shape.name;

	std::vector<Mesh::Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<Mesh::SubMesh> subMeshes;
	std::unordered_map<CornerKey, uint32_t, CornerKeyHash> uniqueVertices;

	for (const auto materialIndex : materialOrder) {
		Mesh::SubMesh subMesh{};
		subMesh.indexStart = static_cast<uint32_t>(indices.size());

		for (const auto *triangle : triangleByMaterial[materialIndex]) {
			constexpr int kCornerOrder[] = {0, 2, 1};
			for (const auto &c : kCornerOrder) {
				const auto &corner = triangle->corners[c];
				const CornerKey key = {corner.position, corner.texcoord, corner.normal};
				const auto [it, inserted] = uniqueVertices.try_emplace(key, static_cast<uint32_t>(vertices.size()));
				if (inserted) {
					vertices.push_back(MakeVertex(data, corner));
				}
				indices.push_back(it->second);
			}
		}

		subMesh.indexCount = static_cast<uint32_t>(indices.size()) - subMesh.indexStart;
		subMeshes.push_back(subMesh);
		node.materials.push_back(materialIndex >= 0 ? materials[materialIndex] : fallbackMaterial);
	}

	node.mesh = std::make_shared<Mesh>();
	node.mesh->SetName(shape.name);
	node.mesh->SetVertices(std::move(vertices));
	node.mesh->SetIndices(std::move(indices));
	node.mesh->SetSubMeshes(std::move(subMeshes));
	node.mesh->Upload();

	return node;
}

} // namespace



std::shared_ptr<Model> ou::Model::LoadObj(std::string_view fileName) {
	const auto assets = FindAssetsFolder();
	if (!assets) {
		Log::Error("Assets フォルダが見つかりません。");
		return nullptr;
	}

	const auto path = *assets / Utf8ToWide(fileName);
	const auto data = ParseObjFile(path);
	if (!data) { return nullptr; }

	const auto slash = fileName.find_last_of("/\\");
	const auto directory = (slash == std::string_view::npos) ? "" : std::string(fileName.substr(0, slash + 1));

	const auto materials = CreateMaterials(*data, directory);
	const auto fallbackMaterial = Material::Create();

	auto model = std::make_shared<Model>();
	model->SetName(ToUtf8(path.stem()));
	for (const auto &shape : data->shapes) {
		if (shape.triangles.empty()) { continue; }
		model->nodes_.push_back(BuildNode(*data, shape, materials, fallbackMaterial));
	}

	Log::Info("モデルを読み込みました: {} (ノード {} 個)", fileName, model->nodes_.size());
	return model;
}


GameObject *ou::Model::Instantiate(Scene *scene) const {
	auto *root = scene->CreateGameObject(GetName());

	for (const auto &node : nodes_) {
		auto *child = scene->CreateGameObject(node.name);
		child->GetTransform()->SetParent(root->GetTransform());

		auto *renderer = child->AddComponent<MeshRenderer>();
		renderer->SetMesh(node.mesh);
		renderer->SetMaterials(node.materials);
	}

	return root;
}


const std::vector<Model::Node> &ou::Model::GetNodes() const {
	return nodes_;
}
