#include "Renderer.h"
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


void ou::Renderer::SetMaterial(std::shared_ptr<Material> material) {
	if (materials_.empty()) {
		materials_.push_back(std::move(material));
	} else {
		materials_[0] = std::move(material);
	}
}


const std::shared_ptr<Material> &ou::Renderer::GetMaterial() const {
	static const std::shared_ptr<Material> kNull = nullptr;
	return materials_.empty() ? kNull : materials_[0];
}


void ou::Renderer::SetMaterials(std::vector<std::shared_ptr<Material>> materials) {
	materials_ = std::move(materials);
}


const std::vector<std::shared_ptr<Material>> &ou::Renderer::GetMaterials() const {
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
