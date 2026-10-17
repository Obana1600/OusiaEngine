#include "ParticleSystem.h"
#include "Core/Time.h"
#include "Math/Mathf.h"
#include "Math/Random.h"
#include "Scene/Transform.h"

#include <utility>

using namespace ou;


namespace {

constexpr float kGravity = -9.81f;

} // namespace



float ou::FloatRange::Evaluate() const {
	if (min == max) { return min; }
	return (min < max) ? Random::Range(min, max) : Random::Range(max, min);
}


Color ou::ColorRange::Evaluate() const {
	const auto t = Random::Value();
	return Color::Lerp(min, max, t);
}


void ou::ParticleSystem::Awake() {
	if (main_.playOnAwake) { Play(); }
}

void ou::ParticleSystem::Update() {
	const auto deltaTime = Time::GetDeltaTime();

	UpdateParticles(deltaTime);
	if (isPlaying_) {
		UpdateEmission(deltaTime);
	}
}


void ou::ParticleSystem::Play() {
	isPlaying_ = true;
	time_ = 0.0f;
	emitAccumulator_ = 0.0f;
}


void ou::ParticleSystem::Stop() {
	isPlaying_ = false;
}


void ou::ParticleSystem::Clear() {
	particles_.clear();
}


void ou::ParticleSystem::Emit(uint32_t count) {
	for (uint32_t i = 0; i < count; i++) {
		if (particles_.size() >= main_.maxParticles) { return; }
		particles_.push_back(MakeParticle());
	}
}


bool ou::ParticleSystem::IsPlaying() const {
	return isPlaying_;
}


ParticleSystem::MainModule &ou::ParticleSystem::GetMain() {
	return main_;
}

const ParticleSystem::MainModule &ou::ParticleSystem::GetMain() const {
	return main_;
}

ParticleSystem::EmissionModule &ou::ParticleSystem::GetEmission() {
	return emission_;
}

ParticleSystem::ShapeModule &ou::ParticleSystem::GetShape() {
	return shape_;
}


ParticleSystem::ColorOverLifetimeModule &ou::ParticleSystem::GetColorOverLifetime() {
	return colorOverLifetime_;
}


ParticleSystem::SizeOverLifetimeModule &ou::ParticleSystem::GetSizeOverLifetime() {
	return sizeOverLifetime_;
}


const std::vector<ParticleSystem::Particle> &ou::ParticleSystem::GetParticles() const {
	return particles_;
}


Color ou::ParticleSystem::EvaluateColor(const Particle &particle) const {
	if (!colorOverLifetime_.enabled) { return particle.startColor; }

	const auto t = particle.age / particle.startLifetime;
	return particle.startColor * Color::Lerp(colorOverLifetime_.start, colorOverLifetime_.end, t);
}


float ou::ParticleSystem::EvaluateSize(const Particle &particle) const {
	if (!sizeOverLifetime_.enabled) { return particle.startSize; }

	const auto t = particle.age / particle.startLifetime;
	return particle.startSize * Mathf::Lerp(sizeOverLifetime_.start, sizeOverLifetime_.end, t);
}


void ou::ParticleSystem::UpdateParticles(float deltaTime) {
	const Vector3 gravity = {0.0f, kGravity * main_.gravityModifier, 0.0f};

	// パーティクルの更新
	for (auto &particle : particles_) {
		particle.age += deltaTime;
		particle.velocity += gravity * deltaTime;
		particle.position += particle.velocity * deltaTime;
	}

	// パーティクルの削除
	std::erase_if(particles_, [](const auto &particle) {
		return particle.age >= particle.startLifetime;
	});
}


void ou::ParticleSystem::UpdateEmission(float deltaTime) {
	const auto prev = time_;
	time_ += deltaTime;

	if (emission_.enabled) {
		emitAccumulator_ += emission_.rateOverTime * deltaTime;
		const auto count = static_cast<uint32_t>(emitAccumulator_);
		emitAccumulator_ -= static_cast<float>(count);
		Emit(count);

		for (const auto &burst : emission_.bursts) {
			if (prev <= burst.time && burst.time < time_) {
				Emit(burst.count);
			}
		}
	}

	if (main_.duration <= 0.0f || time_ < main_.duration) {
		return;
	}

	if (!main_.looping) {
		isPlaying_ = false;
		return;
	}

	time_ = Mathf::Fmod(time_, main_.duration);
	if (emission_.enabled) {
		for (const auto &burst : emission_.bursts) {
			if (burst.time < time_) { Emit(burst.count); }
		}
	}
}


ParticleSystem::Particle ou::ParticleSystem::MakeParticle() const {
	auto position = Vector3::Zero;
	auto direction = Vector3::Up;

	if (shape_.enabled) {
		switch (shape_.shapeType) {
			case ShapeType::Sphere: {
				const auto inner = 1.0f - Mathf::Clamp01(shape_.radiusThickness);
				const auto innerCubed = inner * inner * inner;
				const auto r = Mathf::Pow(Mathf::Lerp(innerCubed, 1.0f, Random::Value()), 1.0f / 3.0f);

				direction = Random::OnUnitSphere();
				position = direction * (shape_.radius * r);
				break;
			}

			case ShapeType::Cone: {
				const auto disk = Random::InsideUnitCircle();
				const auto angle = shape_.angle * Mathf::Deg2Rad;
				position = {disk.x * shape_.radius, 0.0f, disk.y * shape_.radius};
				direction = Vector3(disk.x * Mathf::Sin(angle),
									Mathf::Cos(angle),
									disk.y * Mathf::Sin(angle))
								.Normalize();
				break;
			}
		}
	}

	Particle particle{};
	particle.position = position;
	particle.velocity = direction * main_.startSpeed.Evaluate();
	particle.startColor = main_.startColor.Evaluate();
	particle.startSize = main_.startSize.Evaluate();
	particle.startLifetime = main_.startLifetime.Evaluate();

	// ワールド空間でシミュレートする
	if (main_.simulationSpace == SimulationSpace::World) {
		const auto &localToWorld = GetTransform()->GetLocalToWorldMatrix();
		particle.position = localToWorld.MultiplyPoint3x4(particle.position);
		particle.velocity = localToWorld.MultiplyVector(particle.velocity);
	}

	return particle;
}
