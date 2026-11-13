#include "GameScene.h"
#include "Animation/Skeletal/SkeletalAnimationClip.h"
#include "Animation/Skeletal/SkeletalAnimator.h"
#include "Asset/AssetDatabase.h"
#include "Core/Application.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Input/Input.h"
#include "Platform/Screen.h"
#include "Serialize/TypeRegistry.h"

#include <utility>
#include "Math/Mathf.h"
#include "Physics/BoxCollider.h"
#include "Physics/Rigidbody.h"
#include "Physics/SphereCollider.h"
#include "Render/Asset/Model.h"
#include "Render/Camera.h"
#include "Render/Light.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"

using namespace ou;


// エディタの確認用: Edit では止まっていて、Play で Fox が走り、球が落ちる
void ou::BuildGameScene(Scene &scene) {
	// Play で JSON から作り直されても残るように登録する
	static bool isRegistered = false;
	if (!std::exchange(isRegistered, true)) {
		TypeRegistry::Register<InputProbe>("InputProbe");

		// 確認用（オイラー角の順番を Unity の Z → X → Y にした）
		const auto deg = [](float x, float y, float z) { return Vector3{x, y, z} * Mathf::Deg2Rad; };
		const auto q = Quaternion::Euler(deg(30.0f, 45.0f, 60.0f));
		Log::Info("TEST Euler(30,45,60) = ({:.4f}, {:.4f}, {:.4f}, {:.4f})", q.x, q.y, q.z, q.w);

		const auto show = [](const char *label, const Vector3 &v) {
			Log::Info("TEST {} = ({:.3f}, {:.3f}, {:.3f})", label, v.x, v.y, v.z);
		};
		show("Euler(0,90,0) * forward", Quaternion::Euler(deg(0, 90, 0)) * Vector3::Forward);
		show("Euler(90,0,0) * forward", Quaternion::Euler(deg(90, 0, 0)) * Vector3::Forward);
		show("Euler(0,0,90) * right", Quaternion::Euler(deg(0, 0, 90)) * Vector3::Right);
		show("Euler(0,90,30) * up", Quaternion::Euler(deg(0, 90, 30)) * Vector3::Up);

		for (const auto &angles : {deg(30, 45, 60), deg(-20, 170, 10), deg(80, -120, 45), deg(90, 30, 0), deg(0, 90, 0)}) {
			const auto back = Quaternion::Euler(angles).EulerAngles() * Mathf::Rad2Deg;
			const auto in = angles * Mathf::Rad2Deg;
			Log::Info("TEST EulerAngles({:.1f}, {:.1f}, {:.1f}) -> ({:.3f}, {:.3f}, {:.3f})", in.x, in.y, in.z, back.x, back.y, back.z);
		}
	}

	auto *camera = scene.CreateGameObject("Main Camera");
	camera->AddComponent<Camera>();
	camera->AddComponent<InputProbe>();
	camera->GetTransform()->SetLocalPosition({0.0f, 1.0f, -4.0f});

	auto *light = scene.CreateGameObject("Directional Light");
	light->AddComponent<Light>()->SetType(LightType::Directional);
	light->GetTransform()->SetLocalEulerAngles(Vector3{50.0f, -30.0f, 0.0f} * Mathf::Deg2Rad);

	// 見えない地面
	auto *ground = scene.CreateGameObject("Ground");
	ground->GetTransform()->SetLocalPosition({0.0f, -0.5f, 0.0f});
	ground->AddComponent<BoxCollider>()->SetSize({20.0f, 1.0f, 20.0f});

	if (const auto fox = AssetDatabase::LoadAssetAtPath<Model>("Assets/_test/Fox/Fox.gltf")) {
		auto *root = fox->Instantiate(&scene);
		root->GetTransform()->SetLocalPosition({-0.8f, 0.0f, 0.0f});
		root->GetTransform()->SetLocalScale(Vector3::One * 0.012f);
		root->GetTransform()->SetLocalEulerAngles(Vector3{0.0f, 90.0f, 0.0f} * Mathf::Deg2Rad);
		auto *animator = root->GetComponent<SkeletalAnimator>();
		animator->SetClip(fox->FindAnimationClip("Run"));
		animator->SetWrapMode(WrapMode::Loop);
	}

	if (const auto sphere = AssetDatabase::LoadAssetAtPath<Model>("Assets/sphere/sphere.obj")) {
		auto *ball = sphere->Instantiate(&scene);
		ball->SetName("Ball");
		ball->GetTransform()->SetLocalPosition({0.8f, 2.0f, 0.0f});
		ball->GetTransform()->SetLocalScale(Vector3::One * 0.3f);
		ball->AddComponent<SphereCollider>();
		ball->AddComponent<Rigidbody>();
	}
}


void ou::InputProbe::Update() {
	if (Input::GetKeyDown(KeyCode::Space)) { Log::Info("TEST Space down"); }
	if (Input::GetKeyDown(KeyCode::A)) { Log::Info("TEST A down"); }
	if (Input::GetMouseButtonDown(0)) {
		const auto position = Input::GetMousePosition();
		Log::Info("TEST click ({}, {}) screen {}x{}", position.x, position.y, Screen::GetWidth(), Screen::GetHeight());
	}
}


void ou::QuitAfterFrames::Update() {
	if (Time::GetTime() >= 1.0f) { Application::Quit(); }
}
