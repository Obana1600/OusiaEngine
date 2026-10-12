#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Math/Matrix4x4.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Scene/Object.h"

#include <cstdint>
#include <memory>


namespace ou {

class Material: public Object {
public: // インナークラス

	// ライティング方法
	enum class LightingMode: uint32_t {
		None,
		Lambert,
		HalfLambert,
	};

	// 定数バッファ: b1
	struct PerMaterial {
		Matrix4x4 uvTransform;
		Vector4 color;
		uint32_t textureIndex_;
		LightingMode lightingMode;
		uint32_t padding[2];
	};


public: // 静的メンバ関数

	static void StaticInitialize(const RenderPassInfo &renderPass);
	static void StaticFinalize();

	static void BindPipelineState(CommandList cmd);

	static std::shared_ptr<Material> Create();


public: // メンバ関数

	PerMaterial MakePerMaterial() const;

	void SetColor(const Vector4 &color);
	const Vector4 &GetColor() const;

	void SetTextureHandle(uint32_t textureHandle);
	uint32_t GetTextureHandle() const;

	void SetUvOffset(const Vector2 &uvOffset);
	const Vector2 &GetUvOffset() const;

	void SetUvRotation(float uvRotation);
	float GetUvRotation() const;

	void SetUvScale(const Vector2 &uvScale);
	const Vector2 &GetUvScale() const;

	void SetLightingMode(LightingMode lightingMode);
	LightingMode GetLightingMode() const;


private: // 静的メンバ変数

	static inline Shader sVs_{};
	static inline Shader sPs_{};
	static inline PipelineState sPso_{};


private: // メンバ変数

	Vector4 color_ = Vector4::One;
	uint32_t textureHandle_ = 0;

	Vector2 uvOffset_{};
	float uvRotation_ = 0.0f;
	Vector2 uvScale_ = Vector2::One;

	LightingMode lightingMode_ = LightingMode::HalfLambert;
};

} // namespace ou
