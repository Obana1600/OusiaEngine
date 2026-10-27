#include "BossAreaAttackAction.h"
#include "Boss.h"
#include "FilledRing.h"
#include "Player.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"
#include "Tuning.h"

using namespace ou;


namespace {

constexpr float kRadius = 6.0f;
constexpr float kChargeTime = 1.4f;
constexpr float kBlastTime = 0.25f;
constexpr float kAfterWait = 0.4f;

const Color kOutlineColor = {1.0f, 0.3f, 0.2f, 0.9f};
const Color kFillColor = {1.0f, 0.3f, 0.2f, 0.3f};
const Color kBlastColor = {1.0f, 0.9f, 0.6f, 0.7f};

FilledRing *CreateRing(Scene &scene, Transform *parent, float inner, float outer, const Color &color) {
	auto *object = scene.CreateGameObject("Area Attack Ring");
	object->GetTransform()->SetParent(parent);
	// ボスより奥、予兆線より手前に描く
	object->GetTransform()->SetLocalPosition({0.0f, 0.0f, 0.4f});
	auto *ring = object->AddComponent<FilledRing>();
	ring->SetInnerRadius(inner);
	ring->SetOuterRadius(outer);
	ring->SetColor(color);
	return ring;
}

} // namespace



void ou::BossAreaAttackAction::Begin(Boss &boss) {
	auto &scene = boss.GetScene();
	root_ = scene.CreateGameObject("Area Attack");
	const auto position = boss.GetPosition();
	root_->GetTransform()->SetLocalPosition({position.x, position.y, 0.0f});

	// 範囲の縁（常に一周）と、中から外へ満ちていく円盤
	outline_ = CreateRing(scene, root_->GetTransform(), kRadius - 0.15f, kRadius, kOutlineColor);
	fill_ = CreateRing(scene, root_->GetTransform(), 0.0f, kRadius, kFillColor);
	fill_->SetFillAmount(0.0f);
}


bool ou::BossAreaAttackAction::Update(Boss &boss, float dt) {
	timer_ += dt;

	if (timer_ < kChargeTime) {
		const auto t = timer_ / kChargeTime;
		fill_->SetFillAmount(t);
		boss.SetWarningGlow(t);
		return false;
	}

	if (!hasExploded_) {
		hasExploded_ = true;
		boss.SetWarningGlow(0.0f);
		fill_->SetFillAmount(1.0f);
		fill_->SetColor(kBlastColor);

		auto *player = Player::GetInstance();
		if (player) {
			const auto offset = player->GetPosition() - boss.GetPosition();
			if (offset.Length() <= kRadius + player->GetRadius()) { player->TakeHit(Tuning::kAreaAttackDamage, offset); }
		}
	}

	if (timer_ >= kChargeTime + kBlastTime && root_) {
		Object::Destroy(root_);
		root_ = nullptr;
	}
	return timer_ >= kChargeTime + kBlastTime + kAfterWait;
}


void ou::BossAreaAttackAction::End(Boss &boss) {
	boss.SetWarningGlow(0.0f);
	if (root_) {
		Object::Destroy(root_);
		root_ = nullptr;
	}
}
