#include "MeshRenderer.h"
#include "Render/Asset/Mesh.h"
#include "Serialize/Serializer.h"

#include <utility>

using namespace ou;


void ou::MeshRenderer::Serialize(Serializer &serializer) {
	Renderer::Serialize(serializer);

	serializer.Asset("mesh", mesh_);
}


void ou::MeshRenderer::Draw(Graphics::CommandList cmd) const {
	if (mesh_) { DrawMesh(*mesh_, MakePerDraw(), cmd); }
}


void ou::MeshRenderer::SetMesh(std::shared_ptr<Mesh> mesh) {
	mesh_ = std::move(mesh);
}


const std::shared_ptr<Mesh> &ou::MeshRenderer::GetMesh() const {
	return mesh_;
}
