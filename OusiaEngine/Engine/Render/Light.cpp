#include "Light.h"
#include "Math/Mathf.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"
#include "Serialize/Serializer.h"
#include "Render/Gizmos.h"

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


void ou::Light::OnDrawGizmos() {
	// Unity と同じく、アイコンをライトの色で染める
	Gizmos::DrawIcon(GetTransform()->GetWorldPosition(), "Light", true, Color(color_.r, color_.g, color_.b, 1.0f));
}


void ou::Light::OnDrawGizmosSelected() {
	const auto *transform = GetTransform();
	Gizmos::SetColor(Color(1.0f, 1.0f, 0.53f, 0.7f));
	// 位置と向きだけ使う（大きさは掛けない）
	Gizmos::SetMatrix(Matrix4x4::SRT(Vector3::One, transform->GetWorldRotation(), transform->GetWorldPosition()));

	switch (type_) {
		case LightType::Directional: {
			// 光の向き（+Z）に伸びる、円筒に並んだ線
			constexpr float kRadius = 0.5f;
			constexpr float kLength = 2.0f;
			Gizmos::DrawWireDisc(Vector3::Zero, Vector3::Forward, kRadius);
			for (int i = 0; i < 8; i++) {
				const auto angle = static_cast<float>(i) * 45.0f * Mathf::Deg2Rad;
				const Vector3 start{Mathf::Cos(angle) * kRadius, Mathf::Sin(angle) * kRadius, 0.0f};
				Gizmos::DrawRay(start, Vector3::Forward * kLength);
			}
			Gizmos::DrawRay(Vector3::Zero, Vector3::Forward * kLength);
			break;
		}
		case LightType::Point:
			Gizmos::DrawWireSphere(Vector3::Zero, range_);
			break;
		case LightType::Spot: {
			// 先端から range の先の円まで 4 本の線
			const auto radius = range_ * Mathf::Tan(spotAngle_ * 0.5f * Mathf::Deg2Rad);
			const auto end = Vector3::Forward * range_;
			Gizmos::DrawWireDisc(end, Vector3::Forward, radius);
			for (const auto &side : {Vector3::Right, Vector3::Left, Vector3::Up, Vector3::Down}) {
				Gizmos::DrawLine(Vector3::Zero, end + side * radius);
			}
			Gizmos::DrawLine(Vector3::Zero, end);
			break;
		}
	}
}
