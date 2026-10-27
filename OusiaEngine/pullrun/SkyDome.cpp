#include "SkyDome.h"
#include "Asset/AssetDatabase.h"
#include "Boss.h"
#include "Core/Time.h"
#include "GameAssets.h"
#include "Math/Mathf.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Texture.h"
#include "Render/MeshRenderer.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"

#include <cmath>

using namespace ou;


namespace {

// カメラの遠クリップ（500）に収まる大きさ。球の半径は 0.5 なので直径
constexpr float kDiameter = 600.0f;
// 視線（z 軸）まわりに回す速さ（度/秒）
constexpr float kSpinSpeed = 1.5f;
constexpr float kColorSharpness = 1.5f;

const Color kPhase1Color = {0.8f, 0.85f, 1.0f, 1.0f};
const Color kPhase2Color = {1.0f, 0.45f, 0.4f, 1.0f};

} // namespace



void ou::SkyDome::Start() {
	auto &scene = *GetGameObject()->GetScene();
	auto *renderer = CreateMeshObject(scene, "Sky Sphere", GameAssets::GetInstance()->GetSphere(), Vector3::Zero,
									  Vector3::One * kDiameter, kPhase1Color, true);
	sphere_ = renderer->GetGameObject()->GetTransform();
	sphere_->SetParent(GetGameObject()->GetTransform());

	// 内側から見るので裏面も描き、ほかの物より先に、深度を書かずに描く
	material_ = renderer->GetSharedMaterial();
	material_->SetMainTexture(AssetDatabase::LoadAssetAtPath<Texture>("Assets/Texture/sky.png"));
	material_->SetRenderQueue(Shader::RenderQueue::BackGround);
	Shader::RenderStateOverride override{};
	override.zWrite = false;
	override.cull = Graphics::CullMode::NONE;
	material_->SetRenderStateOverride(override);
	color_ = kPhase1Color;
}


void ou::SkyDome::Update() {
	const auto dt = Time::GetUnscaledDeltaTime();
	angle_ += kSpinSpeed * dt;
	sphere_->SetLocalEulerAngles(Vector3{0.0f, 0.0f, angle_} * Mathf::Deg2Rad);

	const auto *boss = Boss::GetInstance();
	const auto &target = boss && boss->GetPhase() == 2 ? kPhase2Color : kPhase1Color;
	const auto blend = 1.0f - std::exp(-kColorSharpness * dt);
	color_ = {Mathf::Lerp(color_.r, target.r, blend), Mathf::Lerp(color_.g, target.g, blend), Mathf::Lerp(color_.b, target.b, blend), 1.0f};
	material_->SetColor(color_);
}
