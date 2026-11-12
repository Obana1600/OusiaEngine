#include "GltfModelReader.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#pragma warning(push, 0)
#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>
#pragma warning(pop)

using namespace ou;


namespace {

struct CgltfDeleter {
	void operator()(cgltf_data *data) const {
		cgltf_free(data);
	}
};
using CgltfData = std::unique_ptr<cgltf_data, CgltfDeleter>;

std::string NameOr(const char *name, std::string_view fallback) {
	if (!name) { return std::string(fallback); }

	std::string decoded = name;
	decoded.resize(cgltf_decode_string(decoded.data()));
	return decoded;
}

std::vector<float> UnpackFloats(const cgltf_accessor *accessor) {
	if (!accessor) { return {}; }

	std::vector<float> values(accessor->count * cgltf_num_components(accessor->type));
	cgltf_accessor_unpack_floats(accessor, values.data(), values.size());
	return values;
}

void DecomposeMatrix(const float m[16], float translation[3], float rotation[4], float scale[3]) {
	// 座標
	translation[0] = m[12];
	translation[1] = m[13];
	translation[2] = m[14];

	// スケール
	for (int i = 0; i < 3; i++) {
		scale[i] = Mathf::Sqrt(m[i * 4 + 0] * m[i * 4 + 0] +
							   m[i * 4 + 1] * m[i * 4 + 1] +
							   m[i * 4 + 2] * m[i * 4 + 2]);
	}

	// 回転
	const auto r = [&](int row, int col) { return m[col * 4 + row] / scale[col]; };
	const auto trace = r(0, 0) + r(1, 1) + r(2, 2);
	if (trace > 0.0f) {
		const auto s = Mathf::Sqrt(trace + 1.0f) * 2.0f;
		rotation[0] = (r(2, 1) - r(1, 2)) / s;
		rotation[1] = (r(0, 2) - r(2, 0)) / s;
		rotation[2] = (r(1, 0) - r(0, 1)) / s;
		rotation[3] = 0.25f * s;

	} else if (r(0, 0) > r(1, 1) && r(0, 0) > r(2, 2)) {
		const auto s = Mathf::Sqrt(1.0f + r(0, 0) - r(1, 1) - r(2, 2)) * 2.0f;
		rotation[0] = 0.25f * s;
		rotation[1] = (r(0, 1) + r(1, 0)) / s;
		rotation[2] = (r(0, 2) + r(2, 0)) / s;
		rotation[3] = (r(2, 1) - r(1, 2)) / s;

	} else if (r(1, 1) > r(2, 2)) {
		const auto s = Mathf::Sqrt(1.0f + r(1, 1) - r(0, 0) - r(2, 2)) * 2.0f;
		rotation[0] = (r(0, 1) + r(1, 0)) / s;
		rotation[1] = 0.25f * s;
		rotation[2] = (r(1, 2) + r(2, 1)) / s;
		rotation[3] = (r(0, 2) - r(2, 0)) / s;

	} else {
		const auto s = Mathf::Sqrt(1.0f + r(2, 2) - r(0, 0) - r(1, 1)) * 2.0f;
		rotation[0] = (r(0, 2) + r(2, 0)) / s;
		rotation[1] = (r(1, 2) + r(2, 1)) / s;
		rotation[2] = 0.25f * s;
		rotation[3] = (r(1, 0) - r(0, 1)) / s;
	}
}

Matrix4x4 ToLeftHandedMatrix(const float *m) {
	return Matrix4x4(m[0], -m[1], -m[2], -m[3],
					 -m[4], m[5], m[6], m[7],
					 -m[8], m[9], m[10], m[11],
					 -m[12], m[13], m[14], m[15]);
}

// マテリアル
ModelData::MaterialData BuildMaterial(const cgltf_material &material, const std::string &pathUtf8) {
	ModelData::MaterialData result;
	result.name = NameOr(material.name, "Material");
	if (!material.has_pbr_metallic_roughness) { return result; }

	const auto &pbr = material.pbr_metallic_roughness;
	const auto *factor = pbr.base_color_factor;
	result.color = Color(factor[0], factor[1], factor[2], factor[3]).Gamma();

	const auto *texture = pbr.base_color_texture.texture;
	if (!texture || !texture->image) { return result; }

	const auto &image = texture->image;
	if (image->uri && !std::string_view(image->uri).starts_with("data:")) {
		std::string uri = image->uri;
		uri.resize(cgltf_decode_string(uri.data()));
		uri.resize(cgltf_decode_uri(uri.data()));
		result.mainTexture = std::move(uri);

	} else {
		Log::Warning("埋め込みのテクスチャはまだ使えません: {}: {}", pathUtf8, result.name);
	}

	return result;
}

// ボーンウェイト
Mesh::BoneWeight MakeBoneWeight(const float *joints, const float *weights) {
	Mesh::BoneWeight result;
	auto total = 0.0f;
	for (int i = 0; i < 4; i++) {
		result.boneIndices[i] = static_cast<uint32_t>(joints[i]);
		result.weights[i] = weights[i];
		total += weights[i];
	}

	if (!(total > 0.0f)) { return {}; }

	for (auto &weight : result.weights) {
		weight /= total;
	}
	return result;
}

// 面の法線
void BuildFlatNormals(ModelData::MeshData &mesh, uint32_t baseVertex, uint32_t indexStart) {
	const std::vector<Mesh::Vertex> vertices(mesh.vertices.begin() + baseVertex, mesh.vertices.end());
	const std::vector<uint32_t> indices(mesh.indices.begin() + indexStart, mesh.indices.end());
	const auto hasBoneWeights = mesh.boneWeights.size() > baseVertex;
	const std::vector<Mesh::BoneWeight> boneWeights(
		hasBoneWeights ? mesh.boneWeights.begin() + baseVertex : mesh.boneWeights.end(), mesh.boneWeights.end());

	mesh.vertices.resize(baseVertex);
	mesh.indices.resize(indexStart);
	if (hasBoneWeights) { mesh.boneWeights.resize(baseVertex); }

	for (size_t i = 0; i + 2 < indices.size(); i += 3) {
		const auto &a = vertices[indices[i] - baseVertex].position;
		const auto &b = vertices[indices[i + 1] - baseVertex].position;
		const auto &c = vertices[indices[i + 2] - baseVertex].position;
		const auto normal = Vector3::Cross(b - a, c - a).Normalized();

		for (size_t corner = 0; corner < 3; corner++) {
			const auto source = indices[i + corner] - baseVertex;
			auto vertex = vertices[source];
			vertex.normal = normal;

			mesh.indices.push_back(static_cast<uint32_t>(mesh.vertices.size()));
			mesh.vertices.push_back(vertex);
			if (hasBoneWeights) { mesh.boneWeights.push_back(boneWeights[source]); }
		}
	}
}

// メッシュ
ModelData::MeshData BuildMesh(const cgltf_data &data, const cgltf_mesh &mesh, const std::string &pathUtf8) {
	ModelData::MeshData result;
	result.name = NameOr(mesh.name, "Mesh");

	for (cgltf_size p = 0; p < mesh.primitives_count; p++) {
		const auto &primitive = mesh.primitives[p];
		if (primitive.type != cgltf_primitive_type_triangles) {
			Log::Warning("三角形以外のプリミティブは読めません: {}: {}", pathUtf8, result.name);
			continue;
		}

		const auto *positionAccessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0);
		if (!positionAccessor) {
			Log::Warning("位置のないプリミティブは読めません: {}: {}", pathUtf8, result.name);
			continue;
		}

		// 座標 / 法線 / UV / 関節 / ウェイト
		const auto positions = UnpackFloats(positionAccessor);
		const auto normals = UnpackFloats(cgltf_find_accessor(&primitive, cgltf_attribute_type_normal, 0));
		const auto texcoords = UnpackFloats(cgltf_find_accessor(&primitive, cgltf_attribute_type_texcoord, 0));
		const auto joints = UnpackFloats(cgltf_find_accessor(&primitive, cgltf_attribute_type_joints, 0));
		const auto weights = UnpackFloats(cgltf_find_accessor(&primitive, cgltf_attribute_type_weights, 0));
		const auto hasBoneWeights = !joints.empty() && !weights.empty();

		// 頂点
		const auto baseVertex = static_cast<uint32_t>(result.vertices.size());
		if (hasBoneWeights) { result.boneWeights.resize(baseVertex); }
		for (cgltf_size v = 0; v < positionAccessor->count; v++) {
			Mesh::Vertex vertex{};
			vertex.position = {-positions[3 * v], positions[3 * v + 1], positions[3 * v + 2]};
			if (!normals.empty()) {
				vertex.normal = {-normals[3 * v], normals[3 * v + 1], normals[3 * v + 2]};
			}
			if (!texcoords.empty()) {
				vertex.texcoord = {texcoords[2 * v], texcoords[2 * v + 1]};
			}
			result.vertices.push_back(vertex);
			if (hasBoneWeights) { result.boneWeights.push_back(MakeBoneWeight(&joints[4 * v], &weights[4 * v])); }
		}

		// インデックス
		Mesh::SubMesh subMesh{};
		subMesh.indexStart = static_cast<uint32_t>(result.indices.size());
		const auto indexCount = primitive.indices ? primitive.indices->count : positionAccessor->count;
		for (cgltf_size i = 0; i + 2 < indexCount; i += 3) {
			constexpr cgltf_size kCornerOrder[] = {0, 2, 1};
			for (const auto corner : kCornerOrder) {
				const auto index = primitive.indices ? cgltf_accessor_read_index(primitive.indices, i + corner)
													 : i + corner;
				result.indices.push_back(baseVertex + static_cast<uint32_t>(index));
			}
		}
		if (normals.empty()) { BuildFlatNormals(result, baseVertex, subMesh.indexStart); }
		subMesh.indexCount = static_cast<uint32_t>(result.indices.size()) - subMesh.indexStart;

		result.submeshes.push_back(subMesh);
		result.materialIndices.push_back(
			primitive.material ? static_cast<int>(cgltf_material_index(&data, primitive.material)) : -1);
	}

	if (!result.boneWeights.empty()) { result.boneWeights.resize(result.vertices.size()); }
	return result;
}

// ノード
void AddNode(const cgltf_data &data, const cgltf_node &node, int parent, std::vector<int> &nodeIndices, ModelData &out) {
	ModelData::NodeData nodeData;
	nodeData.name = NameOr(node.name, "Node");
	nodeData.parent = parent;
	if (node.mesh) { nodeData.mesh = static_cast<int>(cgltf_mesh_index(&data, node.mesh)); }
	if (node.skin) { nodeData.skin = static_cast<int>(cgltf_skin_index(&data, node.skin)); }

	// 座標 / 回転 / スケール
	float translation[3] = {0.0f, 0.0f, 0.0f};
	float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	float scale[3] = {1.0f, 1.0f, 1.0f};
	if (node.has_matrix) {
		DecomposeMatrix(node.matrix, translation, rotation, scale);
	} else {
		if (node.has_translation) { std::copy_n(node.translation, 3, translation); }
		if (node.has_rotation) { std::copy_n(node.rotation, 4, rotation); }
		if (node.has_scale) { std::copy_n(node.scale, 3, scale); }
	}

	// 左手座標系に変換
	nodeData.position = {-translation[0], translation[1], translation[2]};
	nodeData.rotation = {rotation[0], -rotation[1], -rotation[2], rotation[3]};
	nodeData.scale = {scale[0], scale[1], scale[2]};

	// 子の追加
	const auto index = static_cast<int>(out.nodes.size());
	nodeIndices[cgltf_node_index(&data, &node)] = index;
	out.nodes.push_back(std::move(nodeData));
	for (cgltf_size i = 0; i < node.children_count; i++) {
		AddNode(data, *node.children[i], index, nodeIndices, out);
	}
}

// スキン
ModelData::SkinData BuildSkin(const cgltf_data &data, const cgltf_skin &skin, const std::vector<int> &nodeIndices, const std::string &pathUtf8) {
	ModelData::SkinData result;

	for (cgltf_size i = 0; i < skin.joints_count; i++) {
		const auto node = nodeIndices[cgltf_node_index(&data, skin.joints[i])];

		if (node < 0) {
			Log::Warning("シーンにないジョイントがあるためスキンを使いません: {}: {}", pathUtf8, NameOr(skin.name, "Skin"));
			return {};
		}

		result.joints.push_back(node);
	}

	result.inverseBindMatrices.assign(result.joints.size(), Matrix4x4::Identity);
	const auto matrices = UnpackFloats(skin.inverse_bind_matrices);
	const auto matrixCount = std::min(result.joints.size(), matrices.size() / 16);
	for (size_t i = 0; i < matrixCount; i++) {
		result.inverseBindMatrices[i] = ToLeftHandedMatrix(&matrices[i * 16]);
	}
	return result;
}

void ConvertToLeftHanded(ModelData::AnimationChannelData &channel) {
	using Path = ModelData::AnimationChannelData::Path;

	auto &values = channel.values;
	if (channel.path == Path::Translation) {
		for (size_t i = 0; i + 2 < values.size(); i += 3) {
			values[i] = -values[i];
		}
	} else if (channel.path == Path::Rotation) {
		for (size_t i = 0; i + 3 < values.size(); i += 4) {
			values[i + 1] = -values[i + 1];
			values[i + 2] = -values[i + 2];
		}
	}
}

ModelData::AnimationData BuildAnimation(const cgltf_data &data, const cgltf_animation &animation, const std::vector<int> &nodeIndices, const std::string &pathUtf8) {
	using Channel = ModelData::AnimationChannelData;

	ModelData::AnimationData result;
	result.name = NameOr(animation.name, "Animation");

	for (cgltf_size i = 0; i < animation.channels_count; i++) {
		const auto &channel = animation.channels[i];
		if (!channel.target_node || !channel.sampler) { continue; }

		Channel out;
		switch (channel.target_path) {
			case cgltf_animation_path_type_translation:
				out.path = Channel::Path::Translation;
				break;
			case cgltf_animation_path_type_rotation:
				out.path = Channel::Path::Rotation;
				break;
			case cgltf_animation_path_type_scale:
				out.path = Channel::Path::Scale;
				break;
			default:
				Log::Warning("このアニメーションのチャンネルにはまだ対応していません: {} : {} (path = {})",
							 pathUtf8, result.name, static_cast<int>(channel.target_path));
				continue;
		}

		out.node = nodeIndices[cgltf_node_index(&data, channel.target_node)];
		if (out.node < 0) { continue; }

		const auto &sampler = *channel.sampler;
		switch (sampler.interpolation) {
			default:
			case cgltf_interpolation_type_linear:
				out.interpolation = Channel::Interpolation::Linear;
				break;
			case cgltf_interpolation_type_step:
				out.interpolation = Channel::Interpolation::Step;
				break;
			case cgltf_interpolation_type_cubic_spline:
				out.interpolation = Channel::Interpolation::CubicHermite;
				break;
		}

		out.times = UnpackFloats(sampler.input);
		out.values = UnpackFloats(sampler.output);
		ConvertToLeftHanded(out);

		result.channels.push_back(std::move(out));
	}

	return result;
}

} // namespace



std::optional<ModelData> ou::ReadGltfModel(const std::filesystem::path &path) {
	const auto pathUtf8 = ToUtf8(path);
	const auto pathString = path.string();

	// ファイル読み込み
	cgltf_options options{};
	cgltf_data *raw = nullptr;
	auto result = cgltf_parse_file(&options, pathString.c_str(), &raw);
	if (result != cgltf_result_success) {
		Log::Error("glTF の読み込みに失敗しました: {} (cgltf_result = {})", pathUtf8, static_cast<int>(result));
		return std::nullopt;
	}
	const CgltfData data(raw);

	// バイナリを読む
	result = cgltf_load_buffers(&options, data.get(), pathString.c_str());
	if (result != cgltf_result_success) {
		Log::Error("glTF のバッファの読み込みに失敗しました: {} (cgltf_result = {})", pathUtf8, static_cast<int>(result));
		return std::nullopt;
	}

	result = cgltf_validate(data.get());
	if (result != cgltf_result_success) {
		Log::Error("glTF の中身が正しくありません: {} (cgltf_result = {})", pathUtf8, static_cast<int>(result));
		return std::nullopt;
	}

	// マテリアル / メッシュのパース
	ModelData model;
	for (cgltf_size i = 0; i < data->materials_count; i++) {
		model.materials.push_back(BuildMaterial(data->materials[i], pathUtf8));
	}
	for (cgltf_size i = 0; i < data->meshes_count; i++) {
		model.meshes.push_back(BuildMesh(*data, data->meshes[i], pathUtf8));
	}

	// ノード
	std::vector<int> nodeIndices(data->nodes_count, -1);
	const auto *scene = (data->scene)			   ? data->scene
						: (data->scenes_count > 0) ? &data->scenes[0]
												   : nullptr;
	if (scene) {
		for (cgltf_size i = 0; i < scene->nodes_count; i++) {
			AddNode(*data, *scene->nodes[i], -1, nodeIndices, model);
		}
	} else {
		for (cgltf_size i = 0; i < data->nodes_count; i++) {
			if (data->nodes[i].parent) { continue; }
			AddNode(*data, data->nodes[i], -1, nodeIndices, model);
		}
	}

	// スキン / アニメーション
	for (cgltf_size i = 0; i < data->skins_count; i++) {
		model.skins.push_back(BuildSkin(*data, data->skins[i], nodeIndices, pathUtf8));
	}
	for (cgltf_size i = 0; i < data->animations_count; i++) {
		model.animations.push_back(BuildAnimation(*data, data->animations[i], nodeIndices, pathUtf8));
	}

	return model;
}
