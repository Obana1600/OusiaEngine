#include "DirectionalLight.h"
#include "Scene/Transform.h"
#include "Scene/SceneManager.h"

using namespace ou;



void ou::DirectionalLight::Awake() {
	SceneManager::RegisterLight(this);
}


void ou::DirectionalLight::OnDestroy() {
	SceneManager::UnregisterLight(this);
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
