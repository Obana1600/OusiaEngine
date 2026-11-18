#pragma once

#include "Math/Color.h"
#include "Math/Vector3.h"
#include "Scene/Behavior.h"

#include <cstdint>


namespace ou {

// TODO: Area も追加したい
enum class LightType : uint32_t {
	Spot,
	Directional,
	Point,
};

class Light: public Behavior {
public: // 定数

	static constexpr uint32_t kMaxLights = 16;


public: // インナークラス

	struct ShaderLight {
		Vector3 position;
		LightType type;
		Vector3 direction;
		float range;
		Vector3 color;
		float spotAngleScale;
		float spotAngleOffset;
		float padding[3];
	};

	// 定数バッファ: b3
	struct PerFrame {
		uint32_t lightCount;
		uint32_t padding[3];
		ShaderLight lights[kMaxLights];
	};


public: // メンバ関数

	void Awake() override;
	void OnDestroy() override;
	void Serialize(Serializer &serializer) override;
	void OnDrawGizmos() override;
	void OnDrawGizmosSelected() override;

	ShaderLight MakeShaderLight() const;

	void SetType(LightType type);
	LightType GetType() const;

	void SetColor(const Color &color);
	const Color &GetColor() const;

	void SetIntensity(float intensity);
	float GetIntensity() const;

	void SetRange(float range);
	float GetRange() const;

	void SetSpotAngle(float spotAngle);
	float GetSpotAngle() const;

	void SetInnerSpotAngle(float innerSpotAngle);
	float GetInnerSpotAngle() const;


private: // メンバ変数

	LightType type_ = LightType::Point;
	Color color_ = Color::White;
	float intensity_ = 1.0f;
	float range_ = 10.0f;
	float spotAngle_ = 30.0f;
	float innerSpotAngle_ = 21.80208f;
};

} // namespace ou
