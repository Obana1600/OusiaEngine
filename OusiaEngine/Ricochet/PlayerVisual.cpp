#include "PlayerVisual.h"
#include "Core/Time.h"
#include "GameAssets.h"
#include "Math/Mathf.h"
#include "Math/Random.h"
#include "Player.h"
#include "Render/Asset/Material.h"
#include "Render/MeshRenderer.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"
#include "Tuning.h"

#include <cmath>

using namespace ou;


namespace {

const Color kLowColor = {0.55f, 0.6f, 0.75f, 1.0f};
const Color kMidColor = {0.2f, 1.0f, 0.9f, 1.0f};
const Color kHighColor = {1.0f, 0.75f, 0.2f, 1.0f};
const Color kHitColor = {1.0f, 0.15f, 0.15f, 1.0f};

// Rigidbody の angularDamping に当たる（1 秒で exp(-x) 倍）
constexpr float kAngularDamping = 1.0f;
// 力積から角速度への換算（慣性モーメントの逆数）
constexpr float kImpactSpin = 1.6f;

Color LerpColor(const Color &a, const Color &b, float t) {
	return {Mathf::Lerp(a.r, b.r, t), Mathf::Lerp(a.g, b.g, t), Mathf::Lerp(a.b, b.b, t), Mathf::Lerp(a.a, b.a, t)};
}

} // namespace



void ou::PlayerVisual::Start() {
	player_ = GetGameObject()->GetComponent<Player>();
	auto &scene = *GetGameObject()->GetScene();
	const auto *assets = GameAssets::GetInstance();

	// 箱は当たり判定の球に収まる大きさ
	const auto size = Tuning::kPlayerRadius * 1.5f;
	auto *body = CreateMeshObject(scene, "Player Body", assets->GetCube(), Vector3::Zero, Vector3::One * size, kLowColor, false);
	body_ = body->GetTransform();
	body_->SetParent(GetTransform());
	bodyMaterial_ = body->GetSharedMaterial();

	auto *auraObject = scene.CreateGameObject("Player Aura");
	auto *aura = auraObject->AddComponent<MeshRenderer>();
	aura->SetMesh(assets->GetSphere());
	auraMaterial_ = CreateTransparentMaterial({1.0f, 1.0f, 1.0f, 0.0f});
	aura->SetSharedMaterial(auraMaterial_);
	aura_ = auraObject->GetTransform();
	aura_->SetParent(GetTransform());
	aura_->SetLocalScale(Vector3::One * (Tuning::kPlayerRadius * 2.6f));
}


void ou::PlayerVisual::LateUpdate() {
	if (!player_) { return; }
	const auto dt = Time::GetDeltaTime();
	const auto unscaledDt = Time::GetUnscaledDeltaTime();
	auto *transform = GetTransform();

	const auto position = player_->GetPosition();
	transform->SetLocalPosition({position.x, position.y, 0.0f});

	// 溜め中は引っ張る向きに伸びる
	const auto &pull = player_->GetPull();
	if (pull.IsCharging()) {
		const auto direction = pull.GetPullDirection();
		const auto stretch = pull.GetChargeRatio() * Tuning::kMaxStretch;
		transform->SetLocalRotation(Quaternion::Euler(0.0f, 0.0f, Mathf::Atan2(direction.y, direction.x)));
		transform->SetLocalScale({1.0f + stretch, 1.0f - stretch * 0.4f, 1.0f - stretch * 0.4f});
	} else {
		transform->SetLocalRotation(Quaternion::Identity);
		transform->SetLocalScale(Vector3::One);
	}

	// 3 軸の回転（ぶつかったときの角速度を積分して、減衰させる）
	const auto angularSpeed = angularVelocity_.Length();
	if (angularSpeed > 0.0001f) {
		spin_ = Quaternion::AngleAxis(angularVelocity_ / angularSpeed, angularSpeed * dt) * spin_;
		spin_ = Quaternion::Normalize(spin_);
	}
	angularVelocity_ *= std::exp(-kAngularDamping * dt);
	// 親（伸び）の回転を打ち消して、箱の向きは spin_ だけで決める
	body_->SetLocalRotation(Quaternion::Inverse(transform->GetLocalRotation()) * spin_);

	// 色：速度帯ごと。被弾・無敵中は赤く点滅
	flashTimer_ = std::max(flashTimer_ - unscaledDt, 0.0f);
	hitFlashTimer_ = std::max(hitFlashTimer_ - unscaledDt, 0.0f);
	const auto band = player_->GetBand();
	auto color = ComputeBodyColor(band);
	if (player_->IsInvincible() && std::fmod(Time::GetTime() * 20.0f, 2.0f) < 1.0f) { color = kHitColor; }
	if (hitFlashTimer_ > 0.0f) { color = kHitColor; }
	bodyMaterial_->SetColor(color);

	// オーラ：攻撃判定とアーマーがある間（中速以上）だけ見える。中速帯に入った瞬間に強く光る
	auto auraColor = band == SpeedBand::High ? kHighColor : kMidColor;
	auraColor.a = HasAttack(band) ? 0.3f : 0.0f;
	auraColor.a += flashTimer_ * 2.0f;
	auraMaterial_->SetColor(auraColor);
	aura_->SetLocalScale(Vector3::One * (Tuning::kPlayerRadius * (2.6f + flashTimer_ * 4.0f)));
}


void ou::PlayerVisual::OnImpact(const Vector2 &normal, const Vector2 &impulse) {
	// 箱の角が当たったとみなして、接触点を中心から相手側へ、奥行き方向へもずらす。
	// 奥行きのずれがあるので、xy 平面の力積でも x・y 軸まわりに回る
	const auto halfSize = Tuning::kPlayerRadius * 0.75f;
	const Vector3 contact = {-normal.x * halfSize, -normal.y * halfSize, Random::Range(-halfSize, halfSize)};
	// トルク = 接触点 × 力積。慣性モーメントの逆数はまとめて kImpactSpin にする
	angularVelocity_ += Vector3::Cross(contact, {impulse.x, impulse.y, 0.0f}) * kImpactSpin;
}


void ou::PlayerVisual::OnBandChanged(SpeedBand band) {
	// 再発射できるつなぎの窓（中速帯）に入ったことを光で知らせる
	if (band == SpeedBand::Mid) { flashTimer_ = 0.2f; }
}


void ou::PlayerVisual::OnHit() {
	hitFlashTimer_ = 0.1f;
}


Color ou::PlayerVisual::ComputeBodyColor(SpeedBand band) const {
	switch (band) {
		case SpeedBand::High: return kHighColor;
		case SpeedBand::Mid: return LerpColor(kMidColor, Color::White, flashTimer_ * 3.0f);
		case SpeedBand::Low: return kLowColor;
	}
	return kLowColor;
}
