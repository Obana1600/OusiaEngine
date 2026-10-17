#pragma once

#include "Math/Color.h"
#include "Math/Vector3.h"
#include "Scene/Component.h"

#include <cstdint>
#include <vector>


namespace ou {

struct FloatRange {
	float min = 0.0f;
	float max = 0.0f;

	FloatRange() = default;
	FloatRange(float value): min(value), max(value) {}
	FloatRange(float min, float max): min(min), max(max) {}

	float Evaluate() const;
};

struct ColorRange {
	Color min = Color::White;
	Color max = Color::White;

	ColorRange() = default;
	ColorRange(const Color &color): min(color), max(color) {}
	ColorRange(const Color &min, const Color &max): min(min), max(max) {}

	Color Evaluate() const;
};

class ParticleSystem: public Component {
public: // インナークラス

	enum class SimulationSpace : uint8_t {
		Local, // Transform に連動
		World, //
	};

	// TODO: 図形を増やす
	enum class ShapeType : uint8_t {
		Sphere, // 球
		Cone,	// 円錐
	};

	struct Particle {
		Vector3 position;
		Vector3 velocity;
		Color startColor;
		float startSize = 1.0f;
		float startLifetime = 1.0f;
		float age = 0.0f;
	};

	struct MainModule {
		float duration = 5.0f;
		bool looping = true;
		bool playOnAwake = true;
		FloatRange startLifetime = 5.0f;
		FloatRange startSpeed = 5.0f;
		FloatRange startSize = 1.0f;
		ColorRange startColor;
		float gravityModifier = 0.0f;
		uint32_t maxParticles = 1000;
		SimulationSpace simulationSpace = SimulationSpace::Local;
	};

	struct Burst {
		float time = 0.0f;
		uint32_t count = 30;
	};

	struct EmissionModule {
		bool enabled = true;
		float rateOverTime = 10.0f;
		std::vector<Burst> bursts;
	};

	struct ShapeModule {
		bool enabled = true;
		ShapeType shapeType = ShapeType::Sphere;
		float angle = 25.0f;
		float radius = 1.0f;
		float radiusThickness = 1.0f;
	};

	struct ColorOverLifetimeModule {
		bool enabled = false;
		Color start = Color::White;
		Color end = {1.0f, 1.0f, 1.0f, 0.0f};
	};

	struct SizeOverLifetimeModule {
		bool enabled = false;
		float start = 1.0f;
		float end = 0.0f;
	};


public: // メンバ関数

	void Awake() override;
	void Update() override;

	void Play();
	void Stop();
	void Clear();
	void Emit(uint32_t count);

	bool IsPlaying() const;

	MainModule &GetMain();
	const MainModule &GetMain() const;
	EmissionModule &GetEmission();
	ShapeModule &GetShape();
	ColorOverLifetimeModule &GetColorOverLifetime();
	SizeOverLifetimeModule &GetSizeOverLifetime();

	const std::vector<Particle> &GetParticles() const;

	Color EvaluateColor(const Particle &particle) const;
	float EvaluateSize(const Particle &particle) const;


private: // メンバ変数

	MainModule main_;
	EmissionModule emission_;
	ShapeModule shape_;
	ColorOverLifetimeModule colorOverLifetime_;
	SizeOverLifetimeModule sizeOverLifetime_;

	std::vector<Particle> particles_;

	float time_ = 0.0f;
	float emitAccumulator_ = 0.0f;
	bool isPlaying_ = false;


private: // メンバ関数
	void UpdateParticles(float deltaTime);
	void UpdateEmission(float deltaTime);

	Particle MakeParticle() const;
};

} // namespace ou
