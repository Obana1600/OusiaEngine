#include "Boss.h"
#include "BossAreaAttackAction.h"
#include "BossBarrageAction.h"
#include "BossDashAction.h"
#include "BossGiantShotAction.h"
#include "BossHomingAction.h"
#include "BossMoveAction.h"
#include "BossSpiralAction.h"
#include "BossWeakPoint.h"
#include "Core/Time.h"
#include "GameAssets.h"
#include "Math/Mathf.h"
#include "Math/Random.h"
#include "Projectile.h"
#include "Render/Asset/Material.h"
#include "Render/MeshRenderer.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"
#include "Tuning.h"

#include <algorithm>
#include <vector>

using namespace ou;


namespace {

enum class ActionId : uint8_t {
	Barrage,
	Homing,
	GiantShot,
	Move,
	Dash,
	AreaAttack,
	Spiral,
};

const Color kPhase1Color = {0.35f, 0.4f, 0.55f, 1.0f};
const Color kPhase2Color = {0.55f, 0.2f, 0.3f, 1.0f};
const Color kWarningColor = {1.0f, 0.9f, 0.3f, 1.0f};

Color LerpColor(const Color &a, const Color &b, float t) {
	return {Mathf::Lerp(a.r, b.r, t), Mathf::Lerp(a.g, b.g, t), Mathf::Lerp(a.b, b.b, t), Mathf::Lerp(a.a, b.a, t)};
}

std::unique_ptr<BossAction> CreateAction(ActionId id) {
	switch (id) {
		case ActionId::Barrage: return std::make_unique<BossBarrageAction>();
		case ActionId::Homing: return std::make_unique<BossHomingAction>();
		case ActionId::GiantShot: return std::make_unique<BossGiantShotAction>();
		case ActionId::Move: return std::make_unique<BossMoveAction>();
		case ActionId::Dash: return std::make_unique<BossDashAction>();
		case ActionId::AreaAttack: return std::make_unique<BossAreaAttackAction>();
		case ActionId::Spiral: return std::make_unique<BossSpiralAction>();
	}
	return nullptr;
}

} // namespace



void ou::Boss::Awake() {
	sInstance_ = this;
	hp_ = Tuning::kBossPhase1Hp;
	position_ = {6.0f, 1.0f};
}


void ou::Boss::Start() {
	auto &scene = GetScene();
	auto *body = CreateMeshObject(scene, "Boss Body", GameAssets::GetInstance()->GetSphere(), Vector3::Zero,
								  Vector3::One * (GetRadius() * 2.0f), kPhase1Color, false);
	body->GetTransform()->SetParent(GetTransform());
	bodyMaterial_ = body->GetSharedMaterial();

	weakPoint_ = GetGameObject()->GetComponent<BossWeakPoint>();
}


void ou::Boss::Update() {
	const auto dt = Time::GetDeltaTime();
	hitCooldown_ = std::max(hitCooldown_ - dt, 0.0f);
	damageFlash_ = std::max(damageFlash_ - Time::GetUnscaledDeltaTime(), 0.0f);

	switch (state_) {
		case BossState::Fighting:
			UpdateFighting(dt);
			break;
		case BossState::Transition:
			stateTimer_ -= dt;
			if (stateTimer_ <= 0.0f) {
				state_ = BossState::Fighting;
				idleTimer_ = 1.0f;
			}
			break;
		case BossState::Defeated:
			break;
	}
}


void ou::Boss::LateUpdate() {
	GetTransform()->SetLocalPosition({position_.x, position_.y, 0.0f});
	if (bodyMaterial_) { bodyMaterial_->SetColor(ComputeBodyColor()); }
}


void ou::Boss::OnDestroy() {
	CancelAction();
	if (sInstance_ == this) { sInstance_ = nullptr; }
}


float ou::Boss::GetRadius() const {
	return Tuning::kBossRadius;
}


float ou::Boss::GetMaxHp() const {
	return phase_ == 1 ? Tuning::kBossPhase1Hp : Tuning::kBossPhase2Hp;
}


Scene &ou::Boss::GetScene() const {
	return *GetGameObject()->GetScene();
}


bool ou::Boss::CanTakeDamage() const {
	return state_ == BossState::Fighting && hitCooldown_ <= 0.0f;
}


float ou::Boss::TakeDamage(float amount, const Vector2 &direction) {
	if (!CanTakeDamage()) { return 0.0f; }

	const auto damage = IsWeakPointHit(direction) ? amount * Tuning::kWeakPointMultiplier : amount;
	hp_ -= damage;
	hitCooldown_ = Tuning::kBossHitCooldown;
	damageFlash_ = 0.12f;

	if (hp_ <= 0.0f) {
		hp_ = 0.0f;
		if (phase_ == 1) {
			BeginTransition();
		} else {
			CancelAction();
			Projectile::DestroyAll();
			state_ = BossState::Defeated;
		}
	}
	return damage;
}


bool ou::Boss::IsWeakPointHit(const Vector2 &direction) const {
	return phase_ == 2 && weakPoint_ && weakPoint_->IsHit(direction);
}


void ou::Boss::ReflectDash(const Vector2 &normal) {
	dashVelocity_ = Motion2D::Reflect(dashVelocity_, normal);
}


void ou::Boss::UpdateFighting(float dt) {
	if (action_) {
		if (action_->Update(*this, dt)) {
			CancelAction();
			idleTimer_ = phase_ == 1 ? Tuning::kBossPhase1Interval : Tuning::kBossPhase2Interval;
		}
		return;
	}

	idleTimer_ -= dt;
	if (idleTimer_ <= 0.0f) { ChooseNextAction(); }
}


void ou::Boss::ChooseNextAction() {
	std::vector<ActionId> candidates = {ActionId::Barrage, ActionId::Homing, ActionId::GiantShot, ActionId::Move};
	if (phase_ == 2) { candidates.insert(candidates.end(), {ActionId::Dash, ActionId::AreaAttack, ActionId::Spiral}); }

	// 巨大弾は同時に出せる数まで。同じ行動は続けない
	std::erase_if(candidates, [this](ActionId id) {
		if (id == ActionId::GiantShot && Projectile::CountOf(ProjectileKind::Giant) >= Tuning::kGiantBulletMaxCount) { return true; }
		return static_cast<int>(id) == lastActionIndex_;
	});

	// Random::Range(int, int) は上限も返すので（エンジンの不具合）、Value から選ぶ
	const auto index = std::min(static_cast<size_t>(Random::Value() * static_cast<float>(candidates.size())), candidates.size() - 1);
	const auto id = candidates[index];
	lastActionIndex_ = static_cast<int>(id);
	action_ = CreateAction(id);
	action_->Begin(*this);
}


void ou::Boss::CancelAction() {
	if (!action_) { return; }
	action_->End(*this);
	action_.reset();
	isDashing_ = false;
	warningGlow_ = 0.0f;
}


void ou::Boss::BeginTransition() {
	CancelAction();
	Projectile::DestroyAll();
	phase_ = 2;
	hp_ = Tuning::kBossPhase2Hp;
	state_ = BossState::Transition;
	stateTimer_ = Tuning::kBossTransitionTime;
	lastActionIndex_ = -1;
	if (weakPoint_) { weakPoint_->Activate(); }
}


Color ou::Boss::ComputeBodyColor() const {
	if (damageFlash_ > 0.0f) { return Color::White; }
	auto color = phase_ == 1 ? kPhase1Color : kPhase2Color;
	// 移行中は点滅させる
	if (state_ == BossState::Transition) {
		const auto t = 0.5f + 0.5f * Mathf::Sin(Time::GetTime() * 18.0f);
		return LerpColor(kPhase1Color, kPhase2Color, t);
	}
	if (state_ == BossState::Defeated) { return {0.15f, 0.15f, 0.15f, 1.0f}; }
	return LerpColor(color, kWarningColor, Mathf::Clamp01(warningGlow_));
}
