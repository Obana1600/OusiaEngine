#include "BossParticles.h"
#include "Boss.h"
#include "GameAssets.h"
#include "GameColors.h"
#include "ParticleEffects.h"
#include "ParticleEmitter.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"
#include "Tuning.h"

using namespace ou;


namespace {

constexpr float kGatherRadius = 5.0f;
constexpr float kGatherLifetime = 0.5f;

} // namespace



void ou::BossParticles::Start() {
	boss_ = Boss::GetInstance();
	auto &scene = *GetGameObject()->GetScene();
	const auto *assets = GameAssets::GetInstance();

	// 予兆で集まる力（ボスに付いて動くようローカル空間）
	gather_ = ParticleEmitter::Create(scene, {"Boss Gather", assets->GetDotTexture(), ParticleSystemRenderer::BlendMode::Additive,
											  ParticleSystem::SimulationSpace::Local, 500});
	auto &gatherMain = gather_->GetMain();
	gatherMain.startLifetime = kGatherLifetime;
	// ボスの表面で止まるよう、表面までの距離を寿命で割った速さ
	gatherMain.startSpeed = -(kGatherRadius - Tuning::kBossRadius) / kGatherLifetime;
	gatherMain.startSize = {0.15f, 0.35f};
	gatherMain.startColor = {Color::White, GameColors::kGold};
	gather_->GetShape().shapeType = ParticleSystem::ShapeType::Sphere;
	gather_->GetShape().radius = kGatherRadius;
	gather_->GetShape().radiusThickness = 0.0f;
	gather_->GetColorOverLifetime() = {true, {1.0f, 1.0f, 1.0f, 0.1f}, Color::White};

	// 第二形態の火の粉（ワールド空間で、立ちのぼって残る）
	aura_ = ParticleEmitter::Create(scene, {"Boss Aura", assets->GetGlowTexture(), ParticleSystemRenderer::BlendMode::Additive,
											ParticleSystem::SimulationSpace::World, 400});
	auto &auraMain = aura_->GetMain();
	auraMain.startLifetime = {0.8f, 1.6f};
	auraMain.startSpeed = {0.5f, 1.5f};
	auraMain.startSize = {0.3f, 0.7f};
	auraMain.startColor = {GameColors::kBossDanger, {1.0f, 0.55f, 0.2f, 1.0f}};
	// 負の重力で上へ昇る
	auraMain.gravityModifier = -0.25f;
	auraMain.looping = true;
	aura_->GetEmission().rateOverTime = 40.0f;
	aura_->GetShape().shapeType = ParticleSystem::ShapeType::Sphere;
	aura_->GetShape().radius = Tuning::kBossRadius;
	aura_->GetShape().radiusThickness = 0.0f;
	aura_->GetColorOverLifetime() = {true, Color::White, {1.0f, 1.0f, 1.0f, 0.0f}};
	aura_->GetSizeOverLifetime() = {true, 1.0f, 0.2f};
}


void ou::BossParticles::LateUpdate() {
	if (!boss_) { return; }
	const auto position = boss_->GetPosition();
	gather_->GetTransform()->SetLocalPosition({position.x, position.y, 0.0f});
	aura_->GetTransform()->SetLocalPosition({position.x, position.y, 0.0f});

	const auto isFighting = boss_->GetState() == BossState::Fighting;

	// 予兆が強いほど多く集まる
	const auto glow = boss_->GetWarningGlow();
	if (isFighting && glow > 0.05f) {
		gather_->GetEmission().rateOverTime = 60.0f + 300.0f * glow;
		if (!gather_->IsPlaying()) { gather_->Play(); }
	} else if (gather_->IsPlaying()) {
		gather_->Stop();
	}

	// 形態の移行と撃破は大きくはじける
	if (boss_->GetPhase() != lastPhase_) {
		lastPhase_ = boss_->GetPhase();
		ParticleEffects::PlayExplosion(position, GameColors::kBossDanger, 1.5f);
		ParticleEffects::PlayRing(position, Color::White, 8.0f);
		aura_->Play();
	}
	const auto isDefeated = boss_->GetState() == BossState::Defeated;
	if (isDefeated && !wasDefeated_) {
		ParticleEffects::PlayExplosion(position, GameColors::kGold, 2.5f);
		ParticleEffects::PlayExplosion(position, GameColors::kBossDanger, 2.0f);
		ParticleEffects::PlayRing(position, GameColors::kGold, 12.0f);
		aura_->Stop();
	}
	wasDefeated_ = isDefeated;
}
