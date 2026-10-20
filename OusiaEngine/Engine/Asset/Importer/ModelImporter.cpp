#include "ModelImporter.h"
#include "Asset/AssetDatabase.h"
#include "Asset/Format/GltfModelReader.h"
#include "Asset/Format/ModelData.h"
#include "Asset/Format/ObjModelReader.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Render/Material.h"
#include "Render/Mesh.h"
#include "Render/Model.h"
#include "Render/Texture.h"

#include <cwctype>
#include <optional>
#include <utility>

using namespace ou;


namespace {

std::optional<ModelData> ReadModelData(const std::filesystem::path &path) {
	auto extension = path.extension().wstring();
	for (auto &c : extension) {
		c = static_cast<wchar_t>(std::tolower(c));
	}

	if (extension == L".obj") { return ReadObjModel(path); }
	if (extension == L".gltf" || extension == L".glb") { return ReadGltfModel(path); }

	Log::Error("この形式のモデルにはまだ対応していません: {}", ToUtf8(path));
	return std::nullopt;
}

} // namespace



ou::ModelImporter::ModelImporter(std::string assetPath, std::filesystem::path fullPath)
	: AssetImporter(std::move(assetPath), std::move(fullPath)) {}


std::shared_ptr<Object> ou::ModelImporter::Import() {
	return ImportModel();
}


std::shared_ptr<Model> ou::ModelImporter::ImportModel() {
	auto data = ReadModelData(GetFullPath());

	// マテリアル
	std::vector<std::shared_ptr<Material>> materials;
	for (const auto &source : data->materials) {
		auto material = Material::Create();
		material->SetName(source.name);
		material->SetColor(source.color);
		if (!source.mainTexture.empty()) {
			if (auto texture = AssetDatabase::LoadAssetAtPath<Texture>(ResolveAssetPath(source.mainTexture))) {
				material->SetMainTexture(texture);
			} else {
				Log::Warning("モデルのテクスチャの読み込みに失敗しました: {}: {}", GetAssetPath(), source.mainTexture);
			}
		}
		materials.push_back(material);
	}
	const auto defaultMaterial = Material::Create();
	defaultMaterial->SetName("Default");

	// メッシュ
	std::vector<std::shared_ptr<Mesh>> meshes;
	for (auto &source : data->meshes) {
		// 三角形が 1 つもないメッシュは作らない
		if (source.indices.empty()) {
			meshes.push_back(nullptr);
			continue;
		}

		for (auto &vertex : source.vertices) {
			vertex.position *= globalScale_;
		}

		auto mesh = std::make_shared<Mesh>();
		mesh->SetName(source.name);
		mesh->SetVertices(std::move(source.vertices));
		mesh->SetIndices(std::move(source.indices));
		mesh->SetSubMeshes(std::move(source.submeshes));
		mesh->Upload();
		meshes.push_back(std::move(mesh));
	}

	// ノード
	std::vector<Model::Node> nodes;
	for (const auto &source : data->nodes) {
		Model::Node node;
		node.name = source.name;
		node.position = source.position;
		node.rotation = source.rotation;
		node.scale = source.scale;
		node.parent = source.parent;

		if (source.mesh >= 0 && meshes[source.mesh]) {
			node.mesh = meshes[source.mesh];
			for (const auto materialIndex : data->meshes[source.mesh].materialIndices) {
				auto &material = materialIndex >= 0 ? materials[materialIndex] : defaultMaterial;
				node.materials.push_back(material);
			}
		}

		nodes.push_back(std::move(node));
	}

	auto model = std::make_shared<Model>(std::move(nodes));
	model->SetName(ToUtf8(GetFullPath().stem()));
	Log::Info("モデルの読み込みに成功しました: {} (ノード {} 個  メッシュ {} 個  マテリアル {} 個)",
			  GetAssetPath(), model->GetNodes().size(), meshes.size(), materials.size());
	return model;
}


void ou::ModelImporter::SetGlobalScale(float globalScale) {
	globalScale_ = globalScale;
}


float ou::ModelImporter::GetGlobalScale() const {
	return globalScale_;
}
