#include "Light.h"
#include "Math/Mathf.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"
#include "Serialize/Serializer.h"

using namespace ou;


void ou::Light::Awake() {
	SceneManager::RegisterLight(this);
}


void ou::Light::OnDestroy() {
	SceneManager::UnregisterLight(this);
}


void ou::Light::Serialize(Serializer &serializer) {
	Behavior::Serialize(serializer);

	serializer.Enum("type", type_);
	serializer.Field("color", color_);
	serializer.Field("intensity", intensity_);
	serializer.Field("range", range_);
	serializer.Field("spotAngle", spotAngle_);
	serializer.Field("innerSpotAngle", innerSpotAngle_);
}


Light::ShaderLight ou::Light::MakeShaderLight() const {
	const auto *transform = GetTransform();

	const auto cosOuter = Mathf::Cos(spotAngle_ * 0.5f * Mathf::Deg2Rad);
	const auto cosInner = Mathf::Cos(innerSpotAngle_ * 0.5f * Mathf::Deg2Rad);
	const auto spotAngleScale = 1.0f / Mathf::Max(cosInner - cosOuter, 0.001f);

	const auto linearColor = color_.Linear();

	ShaderLight light{};
	light.position = transform->GetWorldPosition();
	light.type = type_;
	light.direction = transform->GetForward();
	light.range = range_;
	light.color = linearColor * intensity_;
	light.spotAngleScale = spotAngleScale;
	light.spotAngleOffset = -cosOuter * spotAngleScale;

	return light;
}


void ou::Light::SetType(LightType type) {
	type_ = type;
}


LightType ou::Light::GetType() const {
	return type_;
}


void ou::Light::SetColor(const Color &color) {
	color_ = color;
}


const Color &ou::Light::GetColor() const {
	return color_;
}


void ou::Light::SetIntensity(float intensity) {
	intensity_ = intensity;
}


float ou::Light::GetIntensity() const {
	return intensity_;
}


void ou::Light::SetRange(float range) {
	range_ = range;
}


float ou::Light::GetRange() const {
	return range_;
}


void ou::Light::SetSpotAngle(float spotAngle) {
	spotAngle_ = spotAngle;
}


float ou::Light::GetSpotAngle() const {
	return spotAngle_;
}


void ou::Light::SetInnerSpotAngle(float innerSpotAngle) {
	innerSpotAngle_ = innerSpotAngle;
}


float ou::Light::GetInnerSpotAngle() const {
	return innerSpotAngle_;
}
