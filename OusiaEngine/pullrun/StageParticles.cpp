#include "StageParticles.h"
#include "Boss.h"
#include "Core/Time.h"
#include "GameAssets.h"
#include "Math/Mathf.h"
#include "Math/Random.h"
#include "ParticleEmitter.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Tuning.h"

#include <algorithm>
#include <cmath>

using namespace ou;


namespace {

// 線の粒の間隔（途切れて見えない程度に詰める）
constexpr float kLineSpacing = 0.08f;
// 線が入ってこない、ステージ中央の円の半径
constexpr float kInnerRadius = 2.5f;

// 形態ごとの集中線
struct LineStyle {
	float rate;		// 1 秒あたりの本数
	float speed;	// 中央へ向かう速さ
	float lifetime; // 寿命（この間に speed × lifetime だけ進んで消える）
	Color color;
};

const LineStyle kPhase1Style = {100.0f, 15.0f, 1.5f, {0.7f, 0.8f, 1.0f, 0.22f}};
const LineStyle kPhase2Style = {200.0f, 30.0f, 0.75f, {1.0f, 0.35f, 0.25f, 0.4f}};

} // namespace



void ou::StageParticles::Start() {
	// 角度 0・半径 0 の円錐で、エミッタの +y 向きにまっすぐ飛ばす
	lines_ = ParticleEmitter::Create(*GetGameObject()->GetScene(), {"Focus Lines", GameAssets::GetInstance()->GetGlowTexture(),
																	ParticleSystemRenderer::BlendMode::Additive,
																	ParticleSystem::SimulationSpace::World, 8000});
	lines_->GetShape().shapeType = ParticleSystem::ShapeType::Cone;
	lines_->GetShape().angle = 0.0f;
	lines_->GetShape().radius = 0.0f;
	lines_->GetColorOverLifetime() = {true, Color::White, {1.0f, 1.0f, 1.0f, 0.0f}};
}


void ou::StageParticles::Update() {
	const auto *boss = Boss::GetInstance();
	const auto phase = boss ? boss->GetPhase() : 1;
	const auto &style = phase == 1 ? kPhase1Style : kPhase2Style;

	// 第二形態に入った瞬間は、まとめて一気に集める
	if (phase != lastPhase_) {
		lastPhase_ = phase;
		for (int i = 0; i < 80; i++) { SpawnLine(style.speed * 1.5f, style.lifetime, style.color); }
	}

	spawnCarry_ += style.rate * Time::GetDeltaTime();
	while (spawnCarry_ >= 1.0f) {
		spawnCarry_ -= 1.0f;
		SpawnLine(style.speed, style.lifetime, style.color);
	}
}


void ou::StageParticles::SpawnLine(float speed, float lifetime, const Color &color) {
	const auto angle = Random::Range(0.0f, 2.0f * Mathf::PI);
	const Vector2 outward = {Mathf::Cos(angle), Mathf::Sin(angle)};

	// 寿命の間に進んでも中央の円に入らないよう、先頭をその分外に置く。外側はステージの角まで
	const auto travel = speed * lifetime;
	const auto outerRadius = std::sqrt(Tuning::kArenaHalfWidth * Tuning::kArenaHalfWidth + Tuning::kArenaHalfHeight * Tuning::kArenaHalfHeight);
	const auto headDistance = Random::Range(kInnerRadius + travel, std::max(outerRadius, kInnerRadius + travel + 0.1f));
	const auto length = Random::Range(1.5f, 4.0f);
	// ゲームの物より奥（床の手前）を流す
	const auto z = Random::Range(0.5f, 0.9f);

	auto &main = lines_->GetMain();
	main.startSpeed = speed;
	main.startLifetime = lifetime;

	// 両端が細く、真ん中が太い、細長い線にする
	const auto count = std::max(static_cast<int>(std::ceil(length / kLineSpacing)), 2);
	for (int i = 0; i < count; i++) {
		const auto t = static_cast<float>(i) / static_cast<float>(count - 1);
		const auto taper = std::sin(t * Mathf::PI);
		main.startSize = 0.05f + 0.12f * taper;
		auto dotColor = color;
		dotColor.a *= taper;
		main.startColor = dotColor;
		ParticleEmitter::Aim(*lines_, outward * (headDistance + kLineSpacing * static_cast<float>(i)), -outward, z);
		lines_->Emit(1);
	}
}
