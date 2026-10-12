#include "GameScene.h"
#include "Graphics/TextureManager.h"
#include "Input/InputSystem.h"
#include "Math/Mathf.h"
#include "Render/Material.h"
#include "Render/MeshRenderer.h"
#include "Render/Model.h"
#include "Render/ModelManager.h"
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
	scene_ = std::make_unique<Scene>();

	auto *cameraObject = scene_->CreateGameObject("Main Camera");
	cameraObject->AddComponent<Camera>();
	cameraObject->GetTransform()->SetLocalPosition({0.0f, 1.0f, -5.0f});
	cameraObject->GetTransform()->SetLocalEulerAngles({15.0f * Mathf::Deg2Rad, 0.0f, 0.0f});

	const auto teapot = ModelManager::Load("teapot/teapot.obj");
	for (int i = 0; i < 3; i++) {
		auto *object = teapot->Instantiate(scene_.get());
		object->GetTransform()->SetLocalPosition({-4.0f + 4.0f * i, 0.0f, 3.0f});
	}

	switchable_ = scene_->CreateGameObject("Switchable");
	switchable_->AddComponent<Rotator>();
	switchable_->AddComponent<MeshRenderer>();
	SwitchModel("sphere/sphere.obj");

	auto *lightObject = scene_->CreateGameObject("Directional Light");
	lightObject->AddComponent<DirectionalLight>();
	lightObject->GetTransform()->SetLocalEulerAngles({50.0f * Mathf::Deg2Rad, -30.0f * Mathf::Deg2Rad, 0.0f});
}


void GameScene::Update() {
	if (InputSystem::GetInstance()->TriggerKey(DIK_SPACE)) {
		const char *files[] = {"sphere/sphere.obj", "suzanne/suzanne.obj", "teapot/teapot.obj", "bunny/bunny.obj"};
		modelIndex_ = (modelIndex_ + 1) % 4;
		SwitchModel(files[modelIndex_]);
	}

	scene_->Update();
}


void ou::GameScene::Draw(CommandList cmd) const {
	scene_->Render(cmd);
}


void ou::GameScene::SwitchModel(std::string_view fileName) {
	auto *meshRenderer = switchable_->GetComponent<MeshRenderer>();

	const auto model = ModelManager::Load(fileName);
	const auto &node = model->GetNodes().front();
	meshRenderer->SetMesh(node.mesh);
	meshRenderer->SetMaterials(node.materials);
}
