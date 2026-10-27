#pragma once

#include "Math/Vector2.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleSystemRenderer.h"

#include <cstdint>
#include <memory>
#include <string_view>


namespace ou {

// 前方宣言
class Scene;
class Texture;

struct EmitterSettings {
	std::string_view name = "Particle Emitter";
	std::shared_ptr<Texture> texture;
	ParticleSystemRenderer::BlendMode blendMode = ParticleSystemRenderer::BlendMode::Additive;
	ParticleSystem::SimulationSpace simulationSpace = ParticleSystem::SimulationSpace::World;
	uint32_t maxParticles = 500;
};

namespace ParticleEmitter {

// 止まった状態の ParticleSystem を作る（連続して出すなら Play、まとめて出すなら Emit を呼ぶ）
ParticleSystem *Create(Scene &scene, const EmitterSettings &settings);

// エミッタを position に置き、円錐（Cone）の向きを xy 平面の direction に向ける
void Aim(ParticleSystem &emitter, const Vector2 &position, const Vector2 &direction, float z = -0.5f);

// エミッタを position に置き、円錐の角度 90 度で xy 平面の全方向（輪）に出すよう向ける
void AimRing(ParticleSystem &emitter, const Vector2 &position, float z = -0.5f);

} // namespace ParticleEmitter

} // namespace ou
