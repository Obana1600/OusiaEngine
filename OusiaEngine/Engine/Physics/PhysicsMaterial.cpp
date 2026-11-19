#include "PhysicsMaterial.h"
#include "Math/Mathf.h"
#include "Serialize/Serializer.h"

using namespace ou;


void ou::PhysicsMaterial::Serialize(Serializer &serializer) {
	serializer.Field("dynamicFriction", dynamicFriction_);
	serializer.Field("staticFriction", staticFriction_);
	serializer.Field("bounciness", bounciness_);
	serializer.Enum("frictionCombine", frictionCombine_);
	serializer.Enum("bounceCombine", bounceCombine_);

	// 読んだ値はセッターを通らないので、ここで範囲に収める
	if (serializer.IsReading()) {
		SetDynamicFriction(dynamicFriction_);
		SetStaticFriction(staticFriction_);
		SetBounciness(bounciness_);
	}
}


std::shared_ptr<PhysicsMaterial> ou::PhysicsMaterial::Create(const PhysicsMaterial &source) {
	auto material = std::make_shared<PhysicsMaterial>();
	material->SetName(source.GetName());

	material->dynamicFriction_ = source.dynamicFriction_;
	material->staticFriction_ = source.staticFriction_;
	material->bounciness_ = source.bounciness_;
	material->frictionCombine_ = source.frictionCombine_;
	material->bounceCombine_ = source.bounceCombine_;

	return material;
}


void ou::PhysicsMaterial::SetDynamicFriction(float dynamicFriction) {
	dynamicFriction_ = Mathf::Max(dynamicFriction, 0.0f);
}


float ou::PhysicsMaterial::GetDynamicFriction() const {
	return dynamicFriction_;
}


void ou::PhysicsMaterial::SetStaticFriction(float staticFriction) {
	staticFriction_ = Mathf::Max(staticFriction, 0.0f);
}


float ou::PhysicsMaterial::GetStaticFriction() const {
	return staticFriction_;
}


void ou::PhysicsMaterial::SetBounciness(float bounciness) {
	bounciness_ = Mathf::Clamp01(bounciness);
}


float ou::PhysicsMaterial::GetBounciness() const {
	return bounciness_;
}


void ou::PhysicsMaterial::SetFrictionCombine(PhysicsMaterialCombine frictionCombine) {
	frictionCombine_ = frictionCombine;
}


PhysicsMaterialCombine ou::PhysicsMaterial::GetFrictionCombine() const {
	return frictionCombine_;
}


void ou::PhysicsMaterial::SetBounceCombine(PhysicsMaterialCombine bounceCombine) {
	bounceCombine_ = bounceCombine;
}


PhysicsMaterialCombine ou::PhysicsMaterial::GetBounceCombine() const {
	return bounceCombine_;
}
