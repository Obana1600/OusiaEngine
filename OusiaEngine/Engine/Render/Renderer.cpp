#include "Renderer.h"
#include "Asset/AssetDatabase.h"
#include "Graphics/GraphicsDevice.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Mesh.h"
#include "Render/Asset/Shader.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"
#include "Serialize/Serializer.h"

#include <utility>

using namespace ou;


void ou::Renderer::Awake() {
	SceneManager::RegisterRenderer(this);
}


void ou::Renderer::OnDestroy() {
	SceneManager::UnregisterRenderer(this);
}


void ou::Renderer::Serialize(Serializer &serializer) {
	Behavior::Serialize(serializer);

	// 複製したマテリアルは保存できないので None にして運ぶ。読んだものは AwakeFromLoad で付け替える
	serializedMaterials_ = MakeAssetMaterials();
	serializer.Array("materials", serializedMaterials_, [&](std::shared_ptr<Material> &material) {
		serializer.Asset("", material);
	});
}


void ou::Renderer::AwakeFromLoad() {
	Behavior::AwakeFromLoad();

	// 変わっていなければ、複製したマテリアルをそのまま使い続ける
	if (serializedMaterials_ != MakeAssetMaterials()) {
		SetSharedMaterials(serializedMaterials_);
	}
}


int ou::Renderer::GetRenderQueue() const {
	if (materials_.empty() || !materials_.front()) {
		return Shader::RenderQueue::Geometry;
	}
	return materials_.front()->GetRenderQueue();
}


void ou::Renderer::SetSharedMaterial(std::shared_ptr<Material> material) {
	if (materials_.empty()) {
		materials_.push_back(std::move(material));
		isInstanced_.push_back(false);
	} else {
		materials_[0] = std::move(material);
		isInstanced_[0] = false;
	}
}


const std::shared_ptr<Material> &ou::Renderer::GetSharedMaterial() const {
	static const std::shared_ptr<Material> kNull = nullptr;
	return materials_.empty() ? kNull : materials_[0];
}


void ou::Renderer::SetSharedMaterials(std::vector<std::shared_ptr<Material>> materials) {
	materials_ = std::move(materials);
	isInstanced_.assign(materials_.size(), false);
}


const std::vector<std::shared_ptr<Material>> &ou::Renderer::GetSharedMaterials() const {
	return materials_;
}


const std::shared_ptr<Material> &ou::Renderer::GetMaterial() {
	static const std::shared_ptr<Material> kNull = nullptr;
	if (materials_.empty()) { return kNull; }

	InstantiateMaterial(0);
	return materials_[0];
}


const std::vector<std::shared_ptr<Material>> &ou::Renderer::GetMaterials() {
	for (size_t i = 0; i < materials_.size(); i++) {
		InstantiateMaterial(i);
	}
	return materials_;
}


Renderer::PerDraw ou::Renderer::MakePerDraw() const {
	PerDraw constants{};
	constants.world = GetTransform()->GetLocalToWorldMatrix();
	constants.worldInverseTranspose = constants.world.Inverse().Transpose();

	return constants;
}


void ou::Renderer::DrawMesh(Mesh &mesh, const PerDraw &perDraw, Graphics::CommandList cmd) const {
	if (materials_.empty()) { return; }

	auto *device = Graphics::GetDevice();
	device->BindDynamicConstantBuffer(perDraw, 0, cmd);
	mesh.BindBuffers(cmd);

	const auto subMeshCount = static_cast<uint32_t>(mesh.GetSubMeshes().size());
	for (uint32_t i = 0; i < subMeshCount; i++) {
		const auto *material = FindMaterialForSubMesh(i);
		if (!material || !material->GetShader()) { material = Material::sErrorMaterial_.get(); }

		const auto &shader = material->GetShader();
		if (const auto size = shader->GetPropertyBufferSize(); size > 0) {
			const auto properties = device->AllocateGpu(size, cmd);
			material->WritePropertyBuffer(properties.data);
			device->BindConstantBuffer(&properties.buffer, 1, cmd, properties.offset);
		} else {
			device->BindDynamicConstantBuffer(material->MakePerMaterial(), 1, cmd);
		}

		for (size_t pass = 0; pass < shader->GetPassCount(); pass++) {
			const auto *pso = shader->GetPipelineState(
				pass, material->GetRenderStateOverride(), &Mesh::GetInputLayout(), Graphics::PrimitiveTopology::TRIANGLELIST);
			if (!pso) { continue; }

			device->BindPipelineState(pso, cmd);
			mesh.DrawSubMesh(i, cmd);
		}
	}
}


Material *ou::Renderer::FindMaterialForSubMesh(uint32_t subMeshIndex) const {
	if (materials_.empty()) { return nullptr; }

	if (subMeshIndex < materials_.size()) {
		return materials_[subMeshIndex].get();
	}
	return materials_.back().get();
}


std::vector<std::shared_ptr<Material>> ou::Renderer::MakeAssetMaterials() const {
	auto materials = materials_;
	for (auto &material : materials) {
		if (material && !AssetDatabase::FindAssetReference(material.get())) {
			material = nullptr;
		}
	}
	return materials;
}


void ou::Renderer::InstantiateMaterial(size_t index) {
	if (isInstanced_[index] || !materials_[index]) { return; }

	auto instance = Material::Create(*materials_[index]);
	instance->SetName(materials_[index]->GetName() + " (Instance)");
	materials_[index] = std::move(instance);
	isInstanced_[index] = true;
}
