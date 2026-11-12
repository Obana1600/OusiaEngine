#include "ModelImporter.h"
#include "Animation/Skeletal/SkeletalAnimationClip.h"
#include "Asset/AssetDatabase.h"
#include "Asset/Format/GltfModelReader.h"
#include "Asset/Format/ModelData.h"
#include "Asset/Format/ObjModelReader.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Mesh.h"
#include "Render/Asset/Model.h"
#include "Render/Asset/Texture.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cwctype>
#include <functional>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>

using namespace ou;


namespace {

using Channel = ModelData::AnimationChannelData;

size_t GetKeyStride(const Channel &channel, size_t width) {
	return channel.interpolation == Channel::Interpolation::CubicHermite ? width * 3
																		 : width;
}

bool IsValidChannel(const Channel &channel, size_t width) {
	const auto &times = channel.times;
	const auto &values = channel.values;
	if (times.empty() || values.size() != times.size() * GetKeyStride(channel, width)) { return false; }

	const auto isFinite = [](float value) { return std::isfinite(value); };
	if (!std::all_of(times.begin(), times.end(), isFinite)) { return false; }
	if (!std::all_of(values.begin(), values.end(), isFinite)) { return false; }

	return std::adjacent_find(times.begin(), times.end(), std::greater_equal<float>{}) == times.end();
}

std::string MakeUniquePath(const std::string &parentPath, std::string name, std::unordered_set<std::string> &usedPaths) {
	std::replace(name.begin(), name.end(), '/', '_');

	const auto prefix = parentPath.empty() ? std::string{} : parentPath + "/";
	auto path = prefix + name;
	for (int suffix = 1; !usedPaths.insert(path).second; suffix++) {
		path = prefix + name + "_" + std::to_string(suffix);
	}
	return path;
}

bool CanUseSkin(const Mesh &mesh, const ModelData::SkinData &skin) {
	const auto jointCount = skin.joints.size();
	if (jointCount == 0 || mesh.GetBoneWeights().empty()) { return false; }

	const auto &boneWeights = mesh.GetBoneWeights();
	return std::all_of(boneWeights.begin(), boneWeights.end(), [&](const auto &boneWeight) {
		return std::all_of(std::begin(boneWeight.boneIndices), std::end(boneWeight.boneIndices), [&](auto index) {
			return index < jointCount;
		});
	});
}

std::vector<Matrix4x4> MakeBindPoses(const ModelData::SkinData &skin, float globalScale) {
	auto bindPoses = skin.inverseBindMatrices;
	for (auto &bindPose : bindPoses) {
		bindPose.m41 *= globalScale;
		bindPose.m42 *= globalScale;
		bindPose.m43 *= globalScale;
	}
	return bindPoses;
}

float Slope(float time0, float value0, float time1, float value1) {
	return (value1 - value0) / (time1 - time0);
}

std::array<AnimationCurve, 3> BuildVectorCurves(const Channel &channel, float scale) {
	const auto stride = GetKeyStride(channel, 3);
	const auto valueOffset = channel.interpolation == Channel::Interpolation::CubicHermite ? 3
																						   : 0;
	const auto &times = channel.times;

	std::array<AnimationCurve, 3> curves;
	for (size_t c = 0; c < 3; c++) {
		const auto value = [&](size_t key, size_t offset) {
			return channel.values[key * stride + offset + c] * scale;
		};

		std::vector<Keyframe> keys;
		keys.reserve(times.size());
		for (size_t k = 0; k < times.size(); k++) {
			auto inTangent = 0.0f;
			auto outTangent = 0.0f;

			switch (channel.interpolation) {
				case Channel::Interpolation::Step:
					inTangent = Mathf::Infinity;
					outTangent = Mathf::Infinity;
					break;
				case Channel::Interpolation::Linear:
					if (k > 0) { inTangent = Slope(times[k - 1], value(k - 1, 0), times[k], value(k, 0)); }
					if (k + 1 < times.size()) { outTangent = Slope(times[k], value(k, 0), times[k + 1], value(k + 1, 0)); }
					break;
				case Channel::Interpolation::CubicHermite:
					inTangent = value(k, 0);
					outTangent = value(k, 6);
					break;
			}

			keys.emplace_back(times[k], value(k, valueOffset), inTangent, outTangent);
		}

		curves[c] = AnimationCurve(std::move(keys));
	}

	return curves;
}

QuaternionCurve BuildRotationCurve(const Channel &channel) {
	const auto isCubic = channel.interpolation == Channel::Interpolation::CubicHermite;
	const auto stride = GetKeyStride(channel, 4);
	const auto valueOffset = isCubic ? 4
									 : 0;

	std::vector<QuaternionKeyframe> keys(channel.times.size());
	for (size_t k = 0; k < keys.size(); k++) {
		const auto *value = &channel.values[k * stride];
		auto &key = keys[k];

		key.time = channel.times[k];
		key.value = Quaternion(value[valueOffset], value[valueOffset + 1],
							   value[valueOffset + 2], value[valueOffset + 3])
						.Normalized();
		if (isCubic) {
			key.inTangent = Vector4(value[0], value[1], value[2], value[3]);
			key.outTangent = Vector4(value[8], value[9], value[10], value[11]);
		}
	}

	auto interpolation = QuaternionInterpolation::Linear;
	if (channel.interpolation == Channel::Interpolation::Step) { interpolation = QuaternionInterpolation::Step; }
	if (isCubic) { interpolation = QuaternionInterpolation::CubicHermite; }

	return QuaternionCurve(std::move(keys), interpolation);
}


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
	if (!data) { return nullptr; }

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

		AddSubAsset("Material", material);
		materials.push_back(material);
	}
	const auto defaultMaterial = Material::Create();
	defaultMaterial->SetName("Default");
	AddSubAsset("Material", defaultMaterial);

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
		mesh->SetBoneWeights(std::move(source.boneWeights));
		mesh->Upload();

		AddSubAsset("Mesh", mesh);
		meshes.push_back(std::move(mesh));
	}

	// ノード
	std::vector<Model::Node> nodes;
	std::vector<std::string> nodePaths(data->nodes.size());
	std::unordered_set<std::string> usedPaths;
	std::vector<int> meshSkins(data->meshes.size(), -1);
	for (size_t i = 0; i < data->nodes.size(); i++) {
		const auto &source = data->nodes[i];

		const auto parentPath = source.parent >= 0 ? nodePaths[source.parent] : std::string{};
		nodePaths[i] = MakeUniquePath(parentPath, source.name, usedPaths);

		auto name = nodePaths[i].substr(parentPath.empty() ? 0 : parentPath.size() + 1);
		if (name != source.name) {
			Log::Warning("ノードの名前を変更します: {}: {} -> {}", GetAssetPath(), source.name, name);
		}

		Model::Node node;
		node.name = std::move(name);
		node.position = source.position * globalScale_;
		node.rotation = source.rotation;
		node.scale = source.scale;
		node.parent = source.parent;

		if (source.mesh >= 0 && meshes[source.mesh]) {
			node.mesh = meshes[source.mesh];
			for (const auto materialIndex : data->meshes[source.mesh].materialIndices) {
				auto &material = materialIndex >= 0 ? materials[materialIndex] : defaultMaterial;
				node.materials.push_back(material);
			}
			if (source.skin >= 0) {
				const auto &skin = data->skins[source.skin];
				auto &meshSkin = meshSkins[source.mesh];
				if (CanUseSkin(*node.mesh, skin) && (meshSkin < 0 || meshSkin == source.skin)) {
					if (meshSkin < 0) { node.mesh->SetBindPoses(MakeBindPoses(skin, globalScale_)); }
					meshSkin = source.skin;
					node.bones = skin.joints;
				} else {
					Log::Warning("スキンを使えないため、動かないメッシュとして読みます: {}: {}", GetAssetPath(), node.name);
				}
			}
		}
		nodes.push_back(std::move(node));
	}

	// アニメーション
	std::vector<std::shared_ptr<SkeletalAnimationClip>> clips;
	for (const auto &animation : data->animations) {
		std::vector<BoneAnimationTrack> tracks;
		std::vector<int> trackIndices(data->nodes.size(), -1);

		for (const auto &channel : animation.channels) {
			const auto &path = nodePaths[channel.node];
			const auto width = channel.path == Channel::Path::Rotation ? 4u : 3u;
			if (!IsValidChannel(channel, width)) {
				Log::Warning("アニメーションのチャンネルが正しくありません: {}: {}: {}",
							 GetAssetPath(), animation.name, path);
				continue;
			}

			if (trackIndices[channel.node] < 0) {
				trackIndices[channel.node] = static_cast<int>(tracks.size());
				tracks.emplace_back().path = path;
			}

			auto &track = tracks[trackIndices[channel.node]];
			switch (channel.path) {
				case Channel::Path::Translation:
					track.position = BuildVectorCurves(channel, globalScale_);
					break;
				case Channel::Path::Rotation:
					track.rotation = BuildRotationCurve(channel);
					break;
				case Channel::Path::Scale:
					track.scale = BuildVectorCurves(channel, 1.0f);
					break;
			}
		}

		auto clip = std::make_shared<SkeletalAnimationClip>();
		clip->SetName(animation.name);
		for (auto &track : tracks) {
			clip->AddTrack(std::move(track));
		}

		AddSubAsset("SkeletalAnimationClip", clip);
		clips.push_back(std::move(clip));
	}

	auto model = std::make_shared<Model>(std::move(nodes));
	model->SetName(ToUtf8(GetFullPath().stem()));
	model->SetAnimationClips(std::move(clips));
	Log::Info("モデルの読み込みに成功しました: {} (ノード {} 個  メッシュ {} 個  マテリアル {} 個  アニメーション {} 個)",
			  GetAssetPath(), model->GetNodes().size(), meshes.size(), materials.size(), model->GetAnimationClips().size());
	return model;
}


void ou::ModelImporter::SetGlobalScale(float globalScale) {
	globalScale_ = globalScale;
}


float ou::ModelImporter::GetGlobalScale() const {
	return globalScale_;
}
