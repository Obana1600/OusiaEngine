#include "CanvasRenderer.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Texture.h"

#include <utility>

using namespace ou;


void ou::CanvasRenderer::SetColor(const Color &color) {
	color_ = color;
}


const Color &ou::CanvasRenderer::GetColor() const {
	return color_;
}


void ou::CanvasRenderer::SetAlpha(float alpha) {
	color_.a = alpha;
}


float ou::CanvasRenderer::GetAlpha() const {
	return color_.a;
}


void ou::CanvasRenderer::SetMesh(std::vector<UIVertex> vertices, std::vector<uint32_t> indices, std::vector<SubMesh> subMeshes) {
	vertices_ = std::move(vertices);
	indices_ = std::move(indices);
	subMeshes_ = std::move(subMeshes);

	if (subMeshes_.empty()) {
		subMeshes_.push_back({0, static_cast<uint32_t>(indices_.size())});
	}
}


void ou::CanvasRenderer::Clear() {
	vertices_.clear();
	indices_.clear();
	subMeshes_.clear();
}


const std::vector<UIVertex> &ou::CanvasRenderer::GetVertices() const {
	return vertices_;
}


const std::vector<uint32_t> &ou::CanvasRenderer::GetIndices() const {
	return indices_;
}


const std::vector<CanvasRenderer::SubMesh> &ou::CanvasRenderer::GetSubMeshes() const {
	return subMeshes_;
}


void ou::CanvasRenderer::SetMaterialCount(uint32_t materialCount) {
	materials_.resize(materialCount);
}


uint32_t ou::CanvasRenderer::GetMaterialCount() const {
	return static_cast<uint32_t>(materials_.size());
}


void ou::CanvasRenderer::SetMaterial(std::shared_ptr<Material> material, uint32_t index) {
	if (index >= materials_.size()) {
		materials_.resize(index + 1);
	}

	materials_[index] = std::move(material);
}


const std::shared_ptr<Material> &ou::CanvasRenderer::GetMaterial(uint32_t index) const {
	static const std::shared_ptr<Material> kNone;
	return (index < materials_.size()) ? materials_[index] : kNone;
}


void ou::CanvasRenderer::SetTexture(std::shared_ptr<Texture> texture) {
	texture_ = std::move(texture);
}


const std::shared_ptr<Texture> &ou::CanvasRenderer::GetTexture() const {
	return texture_;
}
