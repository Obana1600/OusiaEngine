#include "GameAssets.h"
#include "Math/Mathf.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Mesh.h"
#include "Render/MeshRenderer.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include <cmath>
#include <vector>

using namespace ou;


namespace {

// 半径 0.5 の UV 球（Unity の Sphere と同じ大きさ）
std::shared_ptr<Mesh> CreateSphereMesh(int slices, int stacks) {
	std::vector<Mesh::Vertex> vertices;
	for (int i = 0; i <= stacks; i++) {
		const auto theta = Mathf::PI * static_cast<float>(i) / static_cast<float>(stacks);
		for (int j = 0; j <= slices; j++) {
			const auto phi = 2.0f * Mathf::PI * static_cast<float>(j) / static_cast<float>(slices);
			const Vector3 normal = {std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi)};
			vertices.push_back({normal * 0.5f, normal,
								{static_cast<float>(j) / static_cast<float>(slices), static_cast<float>(i) / static_cast<float>(stacks)}});
		}
	}

	// Mesh::CreateCube と同じく、外から見て時計回り
	std::vector<uint32_t> indices;
	for (int i = 0; i < stacks; i++) {
		for (int j = 0; j < slices; j++) {
			const auto a = static_cast<uint32_t>(i * (slices + 1) + j);
			const auto b = a + static_cast<uint32_t>(slices + 1);
			indices.insert(indices.end(), {a, a + 1, b + 1, a, b + 1, b});
		}
	}

	auto mesh = std::make_shared<Mesh>();
	mesh->SetName("Sphere");
	mesh->SetVertices(std::move(vertices));
	mesh->SetIndices(std::move(indices));
	mesh->Upload();
	return mesh;
}

} // namespace



void ou::GameAssets::Awake() {
	sInstance_ = this;
	sphere_ = CreateSphereMesh(24, 12);
	cube_ = Mesh::CreateCube();
}


void ou::GameAssets::OnDestroy() {
	if (sInstance_ == this) { sInstance_ = nullptr; }
}


MeshRenderer *ou::CreateMeshObject(Scene &scene, std::string_view name, const std::shared_ptr<Mesh> &mesh,
								   const Vector3 &position, const Vector3 &scale, const Color &color, bool unlit) {
	auto *object = scene.CreateGameObject(name);
	object->GetTransform()->SetLocalPosition(position);
	object->GetTransform()->SetLocalScale(scale);

	auto *renderer = object->AddComponent<MeshRenderer>();
	renderer->SetMesh(mesh);
	auto material = Material::Create();
	material->SetColor(color);
	if (unlit) { material->SetLightingMode(Material::LightingMode::None); }
	renderer->SetSharedMaterial(material);
	return renderer;
}


std::shared_ptr<Material> ou::CreateTransparentMaterial(const Color &color) {
	auto material = Material::Create();
	material->SetLightingMode(Material::LightingMode::None);
	material->SetRenderQueue(Shader::RenderQueue::Transparent);
	material->SetColor(color);

	Shader::RenderStateOverride override{};
	override.srcBlend = Graphics::Blend::SRC_ALPHA;
	override.dstBlend = Graphics::Blend::INV_SRC_ALPHA;
	override.blendOp = Graphics::BlendOp::ADD;
	override.zWrite = false;
	override.cull = Graphics::CullMode::NONE;
	material->SetRenderStateOverride(override);
	return material;
}
