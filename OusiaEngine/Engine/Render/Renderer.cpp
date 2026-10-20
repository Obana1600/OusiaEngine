#include "Renderer.h"
#include "Render/Material.h"
#include "Render/Shader.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"

#include <utility>

using namespace ou;


void ou::Renderer::Awake() {
	GetGameObject()->GetScene()->RegisterRenderer(this);
}


void ou::Renderer::OnDestroy() {
	GetGameObject()->GetScene()->UnregisterRenderer(this);
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


Material *ou::Renderer::FindMaterialForSubMesh(uint32_t subMeshIndex) const {
	if (materials_.empty()) { return nullptr; }

	if (subMeshIndex < materials_.size()) {
		return materials_[subMeshIndex].get();
	}
	return materials_.back().get(); // TODO: ログとかに出す
}


void ou::Renderer::InstantiateMaterial(size_t index) {
	if (isInstanced_[index] || !materials_[index]) { return; }

	auto instance = Material::Create(*materials_[index]);
	instance->SetName(materials_[index]->GetName() + " (Instance)");
	materials_[index] = std::move(instance);
	isInstanced_[index] = true;
}
