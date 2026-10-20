#include "MeshRenderer.h"
#include "Graphics/GraphicsDevice.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Mesh.h"

#include <utility>

using namespace ou;


void ou::MeshRenderer::Draw(Graphics::CommandList cmd) const {
	if (!mesh_) { return; }

	auto *device = Graphics::GetDevice();
	device->BindDynamicConstantBuffer(MakePerDraw(), 0, cmd);
	mesh_->BindBuffers(cmd);

	const auto subMeshCount = static_cast<uint32_t>(mesh_->GetSubMeshes().size());
	for (uint32_t i = 0; i < subMeshCount; i++) {
		const auto *material = FindMaterialForSubMesh(i);
		if (!material || !material->GetShader()) { continue; }

		device->BindDynamicConstantBuffer(material->MakePerMaterial(), 1, cmd);


		const auto &shader = *material->GetShader();
		for (size_t pass = 0; pass < shader.GetPassCount(); pass++) {
			const auto *pso = shader.GetPipelineState(pass, material->GetRenderStateOverride(), &Mesh::GetInputLayout(), Graphics::PrimitiveTopology::TRIANGLELIST);
			if (!pso) { continue; }

			device->BindPipelineState(pso, cmd);
			mesh_->DrawSubMesh(i, cmd);
		}
	}
}


void ou::MeshRenderer::SetMesh(std::shared_ptr<Mesh> mesh) {
	mesh_ = std::move(mesh);
}


const std::shared_ptr<Mesh> &ou::MeshRenderer::GetMesh() const {
	return mesh_;
}
