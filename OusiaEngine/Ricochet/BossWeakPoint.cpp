#include "BossWeakPoint.h"
#include "Boss.h"
#include "Core/Time.h"
#include "GameAssets.h"
#include "Math/Mathf.h"
#include "Math/Random.h"
#include "Render/Asset/Material.h"
#include "Render/MeshRenderer.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"
#include "Tuning.h"

using namespace ou;


namespace {

const Color kMarkerColor = {1.0f, 0.95f, 0.2f, 1.0f};
constexpr float kMarkerRadius = 0.55f;

} // namespace



void ou::BossWeakPoint::Start() {
	auto *renderer = CreateMeshObject(*GetGameObject()->GetScene(), "Boss Weak Point", GameAssets::GetInstance()->GetSphere(),
									  Vector3::Zero, Vector3::One * (kMarkerRadius * 2.0f), kMarkerColor, true);
	marker_ = renderer->GetTransform();
	marker_->SetParent(GetTransform());
	markerMaterial_ = renderer->GetSharedMaterial();
	renderer->GetGameObject()->SetActive(isActive_);
}


void ou::BossWeakPoint::Update() {
	if (!isActive_ || !marker_) { return; }

	switchTimer_ -= Time::GetDeltaTime();
	if (switchTimer_ <= 0.0f) { MoveToNewAngle(); }

	// 切り替わる前は点滅して予告する
	auto color = kMarkerColor;
	if (switchTimer_ < Tuning::kWeakPointWarnTime && Mathf::Fmod(Time::GetTime() * 12.0f, 2.0f) < 1.0f) {
		color = {1.0f, 1.0f, 1.0f, 1.0f};
	}
	markerMaterial_->SetColor(color);

	// ボスの表面より少し手前（カメラ側）に置いて、埋もれないようにする
	const auto radius = Tuning::kBossRadius;
	marker_->SetLocalPosition({Mathf::Cos(angle_) * radius, Mathf::Sin(angle_) * radius, -0.3f});
}


void ou::BossWeakPoint::Activate() {
	isActive_ = true;
	MoveToNewAngle();
	if (marker_) { marker_->GetGameObject()->SetActive(true); }
}


bool ou::BossWeakPoint::IsHit(const Vector2 &direction) const {
	if (!isActive_) { return false; }
	const Vector2 weak = {Mathf::Cos(angle_), Mathf::Sin(angle_)};
	const auto cosine = Vector2::Dot(weak, direction.Normalize());
	return cosine >= Mathf::Cos(Tuning::kWeakPointHitAngle);
}


void ou::BossWeakPoint::MoveToNewAngle() {
	// 前の位置から 90〜270 度ずらす
	angle_ += Random::Range(0.5f, 1.5f) * Mathf::PI;
	switchTimer_ = Tuning::kWeakPointSwitchTime;
}
