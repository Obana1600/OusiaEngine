#include "ParticleEffects.h"
#include "GameAssets.h"
#include "Math/Mathf.h"
#include "ParticleEmitter.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include <cstdint>

using namespace ou;


namespace {

// 白から元の色へ寄せる（加算で中心が白く飛ぶ見た目にする）
Color Brighten(const Color &color, float amount) {
	return {Mathf::Lerp(color.r, 1.0f, amount), Mathf::Lerp(color.g, 1.0f, amount), Mathf::Lerp(color.b, 1.0f, amount), color.a};
}

} // namespace



void ou::ParticleEffects::PlayHit(const Vector2 &position, const Vector2 &direction, const Color &color, float strength) {
	if (!sInstance_) { return; }
	const auto s = Mathf::Clamp01(strength);
	sInstance_->EmitFlash(position, Brighten(color, 0.4f), 2.0f + 3.0f * s);
	sInstance_->EmitSparks(position, direction, color, static_cast<int>(16.0f + 34.0f * s), 10.0f + 14.0f * s, 60.0f);
	sInstance_->EmitShards(position, color, static_cast<int>(4.0f + 10.0f * s), 5.0f + 7.0f * s);
}


void ou::ParticleEffects::PlayExplosion(const Vector2 &position, const Color &color, float strength) {
	if (!sInstance_) { return; }
	sInstance_->EmitFlash(position, Brighten(color, 0.5f), 3.0f + 4.0f * strength);
	sInstance_->EmitSparks(position, {0.0f, 1.0f}, color, static_cast<int>(30.0f + 60.0f * strength), 8.0f + 10.0f * strength, 180.0f);
	sInstance_->EmitShards(position, color, static_cast<int>(10.0f + 20.0f * strength), 4.0f + 8.0f * strength);
}


void ou::ParticleEffects::PlaySparks(const Vector2 &position, const Vector2 &direction, const Color &color, int count) {
	if (!sInstance_) { return; }
	sInstance_->EmitSparks(position, direction, color, count, 7.0f, 70.0f);
}


void ou::ParticleEffects::PlayRing(const Vector2 &position, const Color &color, float radius) {
	if (!sInstance_ || !sInstance_->ring_) { return; }
	auto &ring = *sInstance_->ring_;
	constexpr float kLifetime = 0.35f;
	auto &main = ring.GetMain();
	main.startLifetime = kLifetime;
	main.startSpeed = radius / kLifetime;
	main.startSize = {0.25f, 0.4f};
	main.startColor = Brighten(color, 0.3f);
	ParticleEmitter::AimRing(ring, position);
	ring.Emit(static_cast<uint32_t>(24.0f + radius * 8.0f));
}


void ou::ParticleEffects::Awake() {
	sInstance_ = this;
	auto &scene = *GetGameObject()->GetScene();
	const auto *assets = GameAssets::GetInstance();

	// 火花：小さな丸が飛び散り、重力で落ちながら縮む
	sparks_ = ParticleEmitter::Create(scene, {"Sparks", assets->GetDotTexture(), ParticleSystemRenderer::BlendMode::Additive,
											  ParticleSystem::SimulationSpace::World, 3000});
	sparks_->GetMain().gravityModifier = 0.6f;
	sparks_->GetShape().shapeType = ParticleSystem::ShapeType::Cone;
	sparks_->GetShape().radius = 0.05f;
	sparks_->GetColorOverLifetime() = {true, Color::White, {1.0f, 1.0f, 1.0f, 0.0f}};
	sparks_->GetSizeOverLifetime() = {true, 1.0f, 0.1f};

	// 閃光：大きくぼけた丸が一瞬で広がって消える
	flash_ = ParticleEmitter::Create(scene, {"Flash", assets->GetGlowTexture(), ParticleSystemRenderer::BlendMode::Additive,
											 ParticleSystem::SimulationSpace::World, 64});
	flash_->GetShape().enabled = false;
	flash_->GetMain().startSpeed = 0.0f;
	flash_->GetMain().startLifetime = 0.18f;
	flash_->GetColorOverLifetime() = {true, Color::White, {1.0f, 1.0f, 1.0f, 0.0f}};
	flash_->GetSizeOverLifetime() = {true, 0.5f, 1.4f};

	// 破片：ひし形が散って落ちる
	shards_ = ParticleEmitter::Create(scene, {"Shards", assets->GetDiamondTexture(), ParticleSystemRenderer::BlendMode::Additive,
											  ParticleSystem::SimulationSpace::World, 800});
	shards_->GetMain().gravityModifier = 1.0f;
	shards_->GetMain().startLifetime = {0.5f, 0.9f};
	shards_->GetShape().shapeType = ParticleSystem::ShapeType::Sphere;
	shards_->GetShape().radius = 0.2f;
	shards_->GetColorOverLifetime() = {true, Color::White, {1.0f, 1.0f, 1.0f, 0.0f}};
	shards_->GetSizeOverLifetime() = {true, 1.0f, 0.0f};

	// 輪：xy 平面に広がる
	ring_ = ParticleEmitter::Create(scene, {"Ring", assets->GetDotTexture(), ParticleSystemRenderer::BlendMode::Additive,
											ParticleSystem::SimulationSpace::World, 1000});
	ring_->GetShape().shapeType = ParticleSystem::ShapeType::Cone;
	ring_->GetShape().angle = 90.0f;
	ring_->GetShape().radius = 0.01f;
	ring_->GetColorOverLifetime() = {true, Color::White, {1.0f, 1.0f, 1.0f, 0.0f}};
	ring_->GetSizeOverLifetime() = {true, 1.0f, 0.4f};
}


void ou::ParticleEffects::OnDestroy() {
	if (sInstance_ == this) { sInstance_ = nullptr; }
}


void ou::ParticleEffects::EmitSparks(const Vector2 &position, const Vector2 &direction, const Color &color, int count, float speed, float coneAngle) {
	auto &main = sparks_->GetMain();
	main.startLifetime = {0.25f, 0.55f};
	main.startSpeed = {speed * 0.4f, speed};
	main.startSize = {0.12f, 0.3f};
	main.startColor = {Brighten(color, 0.5f), color};
	// 180 度以上は全方向（円錐では表せないので球にする）
	auto &shape = sparks_->GetShape();
	shape.shapeType = coneAngle >= 180.0f ? ParticleSystem::ShapeType::Sphere : ParticleSystem::ShapeType::Cone;
	shape.angle = coneAngle;
	ParticleEmitter::Aim(*sparks_, position, direction);
	sparks_->Emit(static_cast<uint32_t>(count));
}


void ou::ParticleEffects::EmitFlash(const Vector2 &position, const Color &color, float size) {
	auto &main = flash_->GetMain();
	main.startSize = size;
	main.startColor = color;
	ParticleEmitter::Aim(*flash_, position, {0.0f, 1.0f}, -0.8f);
	flash_->Emit(1);
}


void ou::ParticleEffects::EmitShards(const Vector2 &position, const Color &color, int count, float speed) {
	auto &main = shards_->GetMain();
	main.startSpeed = {speed * 0.5f, speed};
	main.startSize = {0.25f, 0.55f};
	main.startColor = {Brighten(color, 0.3f), color};
	ParticleEmitter::Aim(*shards_, position, {0.0f, 1.0f});
	shards_->Emit(static_cast<uint32_t>(count));
}
