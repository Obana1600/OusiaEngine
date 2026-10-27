#include "Projectile.h"
#include "Core/Time.h"
#include "GameAssets.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"
#include "Tuning.h"

#include <algorithm>

using namespace ou;



int ou::Projectile::CountOf(ProjectileKind kind) {
	return static_cast<int>(std::count_if(sAll_.begin(), sAll_.end(), [kind](const Projectile *projectile) {
		return projectile->GetKind() == kind && !projectile->isVanished_;
	}));
}


void ou::Projectile::DestroyAll() {
	// Vanish の中で sAll_ は変わらない（OnDestroy で外れる）が、念のため写してから回す
	const auto all = sAll_;
	for (auto *projectile : all) { projectile->Vanish(); }
}


void ou::Projectile::Awake() {
	sAll_.push_back(this);
}


void ou::Projectile::Update() {
	if (isVanished_) { return; }
	const auto dt = Time::GetDeltaTime();
	age_ += dt;
	if (!Move(dt) || age_ >= lifetime_) {
		Vanish();
		return;
	}
	GetTransform()->SetLocalPosition({position_.x, position_.y, 0.0f});
}


void ou::Projectile::OnDestroy() {
	std::erase(sAll_, this);
	if (hasLight_) {
		hasLight_ = false;
		sLitCount_--;
	}
}


void ou::Projectile::Vanish() {
	if (isVanished_) { return; }
	isVanished_ = true;
	// 当たり判定からはすぐ外す（Destroy はフレームの終わりに反映される）
	std::erase(sAll_, this);
	Object::Destroy(GetGameObject());
}


void ou::Projectile::Setup(const Vector2 &position, const Vector2 &velocity, float radius, float damage) {
	position_ = position;
	velocity_ = velocity;
	radius_ = radius;
	damage_ = damage;
	GetTransform()->SetLocalPosition({position_.x, position_.y, 0.0f});
	GetTransform()->SetLocalScale(Vector3::One * (radius * 2.0f));
}


void ou::Projectile::AttachLight(const Color &color, float intensity, float range) {
	if (hasLight_ || sLitCount_ >= Tuning::kMaxProjectileLights) { return; }
	hasLight_ = true;
	sLitCount_++;
	CreatePointLight(*GetGameObject()->GetScene(), GetTransform(), color, intensity, range);
}


bool ou::Projectile::IsOutsideArena() const {
	return position_.x < -Tuning::kArenaHalfWidth - radius_ || position_.x > Tuning::kArenaHalfWidth + radius_ ||
		   position_.y < -Tuning::kArenaHalfHeight - radius_ || position_.y > Tuning::kArenaHalfHeight + radius_;
}
