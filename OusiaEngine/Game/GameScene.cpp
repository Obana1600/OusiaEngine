#include "GameScene.h"
#include "Graphics/TextureManager.h"
#include "Math/Mathf.h"
#include "Render/Material.h"
#include "Render/Mesh.h"
#include "Render/MeshRenderer.h"
#include "Scene/Camera.h"
#include "Scene/DirectionalLight.h"
#include "Scene/Transform.h"

#include <cassert>

using namespace ou;


namespace {

// Y 軸まわりに回し続ける
class Rotator: public Component {
public:
	void Update() override {
		auto *transform = GetTransform();
		transform->SetLocalRotation(
			transform->GetLocalRotation() * Quaternion::AngleAxis(Vector3::Up, 0.01f));
	}
};

} // namespace


void GameScene::Initialize() {
	const auto texHandle = TextureManager::Load("uvChecker.png");

	scene_ = std::make_unique<Scene>();

	auto *cameraObject = scene_->CreateGameObject("Main Camera");
	cameraObject->AddComponent<Camera>();
	cameraObject->GetTransform()->SetLocalPosition({0.0f, 1.0f, -3.0f});
	cameraObject->GetTransform()->SetLocalEulerAngles({15.0f * Mathf::Deg2Rad, 0.0f, 0.0f});

	auto material = Material::Create();
	material->SetTextureHandle(texHandle);

	auto *cube = scene_->CreateGameObject("Cube");
	auto *renderer = cube->AddComponent<MeshRenderer>();
	renderer->SetMesh(Mesh::CreateCube());
	renderer->SetMaterial(material);
	cube->AddComponent<Rotator>();

	auto *lightObject = scene_->CreateGameObject("Directional Light");
	lightObject->AddComponent<DirectionalLight>();
	lightObject->GetTransform()->SetLocalEulerAngles({50.0f * Mathf::Deg2Rad, -30.0f * Mathf::Deg2Rad, 0.0f});
}


void GameScene::Update() {
	scene_->Update();
}


void ou::GameScene::Draw(CommandList cmd) const {
	scene_->Render(cmd);
}
