#include "PlayerParticles.h"
#include "GameAssets.h"
#include "GameColors.h"
#include "ParticleEmitter.h"
#include "Player.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"
#include "Tuning.h"

#include <cmath>

using namespace ou;


namespace {

// 集まる粒：この半径の球面から、寿命の間に中心へ届く
constexpr float kGatherRadius = 3.0f;
constexpr float kGatherLifetime = 0.35f;

// トレイルの粒の間隔（速くても途切れないよう、通った線の上に並べる）
constexpr float kTrailSpacing = 0.2f;
constexpr float kEmberSpacing = 0.7f;

} // namespace



void ou::PlayerParticles::Start() {
	player_ = Player::GetInstance();
	auto &scene = *GetGameObject()->GetScene();
	const auto *assets = GameAssets::GetInstance();

	// 集まる粒（プレイヤーに付いて動くようローカル空間）
	gather_ = ParticleEmitter::Create(scene, {"Charge Gather", assets->GetDotTexture(), ParticleSystemRenderer::BlendMode::Additive,
											  ParticleSystem::SimulationSpace::Local, 400});
	auto &gatherMain = gather_->GetMain();
	gatherMain.startLifetime = kGatherLifetime;
	gatherMain.startSpeed = -kGatherRadius / kGatherLifetime;
	gatherMain.startSize = {0.12f, 0.28f};
	gatherMain.looping = true;
	gather_->GetShape().shapeType = ParticleSystem::ShapeType::Sphere;
	gather_->GetShape().radius = kGatherRadius;
	gather_->GetShape().radiusThickness = 0.0f;
	// 外では薄く、中心に近づくほど明るく
	gather_->GetColorOverLifetime() = {true, {1.0f, 1.0f, 1.0f, 0.2f}, Color::White};
	gather_->GetSizeOverLifetime() = {true, 0.6f, 1.2f};

	// 光の帯
	trail_ = ParticleEmitter::Create(scene, {"Trail", assets->GetGlowTexture(), ParticleSystemRenderer::BlendMode::Additive,
											 ParticleSystem::SimulationSpace::World, 1500});
	trail_->GetShape().enabled = false;
	auto &trailMain = trail_->GetMain();
	trailMain.startSpeed = 0.0f;
	trailMain.startLifetime = 0.28f;
	trail_->GetColorOverLifetime() = {true, Color::White, {1.0f, 1.0f, 1.0f, 0.0f}};
	trail_->GetSizeOverLifetime() = {true, 1.0f, 0.1f};

	// こぼれる火の粉（少し散って落ちる）
	embers_ = ParticleEmitter::Create(scene, {"Trail Embers", assets->GetDotTexture(), ParticleSystemRenderer::BlendMode::Additive,
											  ParticleSystem::SimulationSpace::World, 600});
	embers_->GetShape().shapeType = ParticleSystem::ShapeType::Sphere;
	embers_->GetShape().radius = 0.2f;
	auto &emberMain = embers_->GetMain();
	emberMain.startSpeed = {0.5f, 2.5f};
	emberMain.startLifetime = {0.4f, 0.8f};
	emberMain.startSize = {0.08f, 0.18f};
	emberMain.gravityModifier = 0.4f;
	embers_->GetColorOverLifetime() = {true, Color::White, {1.0f, 1.0f, 1.0f, 0.0f}};

	if (player_) { lastPosition_ = player_->GetPosition(); }
}


void ou::PlayerParticles::LateUpdate() {
	if (!player_) { return; }
	UpdateGather();
	UpdateTrail();
	lastPosition_ = player_->GetPosition();
}


void ou::PlayerParticles::UpdateGather() {
	const auto position = player_->GetPosition();
	gather_->GetTransform()->SetLocalPosition({position.x, position.y, 0.0f});

	// 手動の溜めと自動突進の溜め。溜めるほど多く、色はご褒美の溜めだけ金色
	const auto &pull = player_->GetPull();
	const auto &autoDash = player_->GetAutoDash();
	const auto isCharging = pull.IsCharging() || autoDash.IsWindingUp();
	if (!isCharging) {
		if (gather_->IsPlaying()) { gather_->Stop(); }
		return;
	}

	const auto ratio = autoDash.IsWindingUp() ? 1.0f : pull.GetChargeRatio();
	gather_->GetEmission().rateOverTime = 40.0f + 260.0f * ratio;
	const auto color = autoDash.IsWindingUp() ? GameColors::kGold : Color{0.6f, 0.95f, 1.0f, 1.0f};
	gather_->GetMain().startColor = {Color::White, color};
	if (!gather_->IsPlaying()) { gather_->Play(); }
}


void ou::PlayerParticles::UpdateTrail() {
	const auto position = player_->GetPosition();
	const auto band = player_->GetBand();
	if (!HasAttack(band)) {
		trailCarry_ = 0.0f;
		emberCarry_ = 0.0f;
		return;
	}

	const auto color = GameColors::ForBand(band);
	trail_->GetMain().startColor = color;
	trail_->GetMain().startSize = band == SpeedBand::High ? 1.1f : 0.7f;
	embers_->GetMain().startColor = {Color::White, color};

	// 前のフレームの位置から今の位置まで、一定の間隔で粒を置く
	const auto delta = position - lastPosition_;
	const auto distance = delta.Length();
	if (distance <= 0.0001f) { return; }
	const auto direction = delta / distance;

	for (trailCarry_ += distance; trailCarry_ >= kTrailSpacing; trailCarry_ -= kTrailSpacing) {
		const auto point = position - direction * (trailCarry_ - kTrailSpacing);
		trail_->GetTransform()->SetLocalPosition({point.x, point.y, 0.1f});
		trail_->Emit(1);
	}
	for (emberCarry_ += distance; emberCarry_ >= kEmberSpacing; emberCarry_ -= kEmberSpacing) {
		const auto point = position - direction * (emberCarry_ - kEmberSpacing);
		embers_->GetTransform()->SetLocalPosition({point.x, point.y, 0.0f});
		embers_->Emit(1);
	}
}
