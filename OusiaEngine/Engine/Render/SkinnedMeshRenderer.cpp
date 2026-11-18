#include "SkinnedMeshRenderer.h"
#include "Diagnostics/Fatal.h"
#include "Diagnostics/Log.h"
#include "Render/Asset/Mesh.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"
#include "Serialize/Serializer.h"

#include <cstring>
#include <utility>

using namespace ou;


void ou::SkinnedMeshRenderer::Serialize(Serializer &serializer) {
	Renderer::Serialize(serializer);

	serializer.Asset("mesh", mesh_);
	serializer.Reference("boneRoot", boneRoot_);
	serializer.Array("bonePaths", bonePaths_, [&](std::string &path) {
		serializer.Field("", path);
	});
}


void ou::SkinnedMeshRenderer::AwakeFromLoad() {
	Renderer::AwakeFromLoad();

	// 骨は次に描くときに探し直す
	boneGeneration_ = kStaleBoneGeneration;
	if (!boneRoot_ || bonePaths_.empty()) { return; }

	// 骨の行列とバッファを、読んだ骨の数で作り直す
	SetBones(boneRoot_, bonePaths_);
	boneGeneration_ = kStaleBoneGeneration;
}


void ou::SkinnedMeshRenderer::Draw(Graphics::CommandList cmd) const {
	if (!mesh_ || mesh_->GetBoneWeights().empty() || bones_.empty()) { return; }
	if (mesh_->GetBindPoses().size() != bones_.size()) { return; }

	if (boneGeneration_ != Scene::GetDestroyGeneration()) { FindBones(); }
	UpdateBoneMatrices();

	auto *device = Graphics::GetDevice();
	const auto &buffer = boneMatrixBuffers_[device->GetBufferIndex()];
	std::memcpy(buffer.mappedData, boneMatrices_.data(), boneMatrices_.size() * sizeof(Matrix4x4));

	auto perDraw = MakePerDraw();
	perDraw.boneMatrixBufferIndex = static_cast<uint32_t>(device->GetDescriptorIndex(&buffer, Graphics::ViewType::SRV));
	perDraw.boneWeightBufferIndex = mesh_->GetBoneWeightBufferIndex();
	DrawMesh(*mesh_, perDraw, cmd);
}


void ou::SkinnedMeshRenderer::SetMesh(std::shared_ptr<Mesh> mesh) {
	mesh_ = std::move(mesh);
}


const std::shared_ptr<Mesh> &ou::SkinnedMeshRenderer::GetMesh() const {
	return mesh_;
}


void ou::SkinnedMeshRenderer::SetBones(Transform *root, std::vector<std::string> bonePaths) {
	if (!mesh_ || mesh_->GetBindPoses().size() != bonePaths.size()) {
		Log::Error("ボーンの数が Mesh のバインドポーズと違います: {} (バインドポーズ = {})",
				   bonePaths.size(), mesh_ ? mesh_->GetBindPoses().size() : 0);
		return;
	}

	boneRoot_ = root;
	bonePaths_ = std::move(bonePaths);
	boneMatrices_.assign(bonePaths_.size(), Matrix4x4::Identity);

	FindBones();
	UpdateBoneMatrices();

	Graphics::GpuBufferDesc desc{};
	desc.size = bonePaths_.size() * sizeof(Matrix4x4);
	desc.heapType = Graphics::HeapType::UPLOAD;
	desc.bindFlags = Graphics::BindFlag::SHADER_RESOURCE;
	desc.miscFlags = Graphics::ResourceMiscFlag::BUFFER_STRUCTURED;
	desc.stride = sizeof(Matrix4x4);

	for (auto &buffer : boneMatrixBuffers_) {
		if (!Graphics::GetDevice()->CreateBuffer(&desc, nullptr, &buffer)) {
			FatalError("ボーンの行列バッファの作成に失敗しました");
		}
	}
}


const std::vector<Matrix4x4> &ou::SkinnedMeshRenderer::GetBoneMatrices() const {
	return boneMatrices_;
}


void ou::SkinnedMeshRenderer::FindBones() const {
	boneGeneration_ = Scene::GetDestroyGeneration();

	bones_.resize(bonePaths_.size());
	for (size_t i = 0; i < bones_.size(); i++) {
		bones_[i] = boneRoot_ ? boneRoot_->Find(bonePaths_[i]) : nullptr;
	}
}


void ou::SkinnedMeshRenderer::UpdateBoneMatrices() const {
	const auto &bindPoses = mesh_->GetBindPoses();
	for (size_t i = 0; i < bones_.size(); i++) {
		if (bones_[i]) { boneMatrices_[i] = bindPoses[i] * bones_[i]->GetLocalToWorldMatrix(); }
	}
}
