#pragma once

#include "Core/Object.h"
#include "Math/Color.h"
#include "Math/Matrix4x4.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Render/Asset/Shader.h"
#include "Render/Asset/Texture.h"

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>


namespace ou {

class Material: public Object {
	friend class Renderer;

public: // インナークラス

	// ライティング方法
	enum class LightingMode : uint32_t {
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

	static void StaticInitialize();
	static void StaticFinalize();

	static std::shared_ptr<Material> Create(std::shared_ptr<Shader> shader = nullptr);
	static std::shared_ptr<Material> Create(const Material &source);


public: // メンバ関数

	PerMaterial MakePerMaterial() const;

	void SetShader(std::shared_ptr<Shader> shader);
	const std::shared_ptr<Shader> &GetShader() const;

	void SetRenderQueue(int renderQueue);
	int GetRenderQueue() const;

	void SetRenderStateOverride(const Shader::RenderStateOverride &renderStateOverride);
	const Shader::RenderStateOverride &GetRenderStateOverride() const;

	void SetColor(const Color &color);
	const Color &GetColor() const;

	void SetMainTexture(std::shared_ptr<Texture> mainTexture);
	const std::shared_ptr<Texture> &GetMainTexture() const;

	void SetUvOffset(const Vector2 &uvOffset);
	const Vector2 &GetUvOffset() const;

	void SetUvRotation(float uvRotation);
	float GetUvRotation() const;

	void SetUvScale(const Vector2 &uvScale);
	const Vector2 &GetUvScale() const;

	void SetLightingMode(LightingMode lightingMode);
	LightingMode GetLightingMode() const;

	bool HasProperty(std::string_view name) const;

	void SetFloat(std::string_view name, float value);
	float GetFloat(std::string_view name) const;

	void SetVector(std::string_view name, const Vector4 &value);
	Vector4 GetVector(std::string_view name) const;

	void SetColor(std::string_view name, const Color &value);
	Color GetColor(std::string_view name) const;

	void WritePropertyBuffer(void *dst) const;


private: // 静的メンバ変数

	static inline std::shared_ptr<Shader> sStandardShader_;
	static inline std::shared_ptr<Texture> sWhiteTexture_;
	static inline std::shared_ptr<Material> sErrorMaterial_;


private: // メンバ変数

	std::shared_ptr<Shader> shader_;
	std::shared_ptr<Texture> mainTexture_;

	int renderQueue_ = -1;
	Shader::RenderStateOverride renderStateOverride_{};

	Vector2 uvOffset_{};
	float uvRotation_ = 0.0f;
	Vector2 uvScale_ = Vector2::One;
	Color color_ = Color::White;
	LightingMode lightingMode_ = LightingMode::HalfLambert;

	std::vector<Vector4> propertyValues_;


private: // メンバ関数

	int32_t FindPropertyIndex(std::string_view name, Shader::PropertyType type) const;
};

} // namespace ou
