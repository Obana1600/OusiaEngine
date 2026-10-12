#include "DirectionalLight.h"
#include "Scene/Transform.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"

using namespace ou;



void ou::DirectionalLight::Awake() {
	GetGameObject()->GetScene()->RegisterLight(this);
}


void ou::DirectionalLight::OnDestroy() {
	GetGameObject()->GetScene()->UnregisterLight(this);
}


DirectionalLight::PerFrame ou::DirectionalLight::MakePerFrame() const {
	PerFrame constants{};
	constants.color = color_;
	constants.direction = GetTransform()->GetForward();
	constants.intensity = intensity_;

	return constants;
}


void ou::DirectionalLight::SetColor(const Vector4 &color) {
	color_ = color;
}


const Vector4 &ou::DirectionalLight::GetColor() const {
	return color_;
}


void ou::DirectionalLight::SetIntensity(float intensity) {
	intensity_ = intensity;
}


float ou::DirectionalLight::GetIntensity() const {
	return intensity_;
}
