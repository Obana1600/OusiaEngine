#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Math/Matrix4x4.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

#include <cstdint>
#include <memory>


namespace ou {

class Sprite {
public: // インナークラス

	struct SpriteConstants {
		Matrix4x4 worldViewProj;
		Matrix4x4 uvTransform;
		Vector4 color;
		Vector4 positionRect;
		Vector4 texcoordRect;
		uint32_t textureIndex;
	};


public: // 静的メンバ関数

	static void StaticInitialize(const RenderPassInfo &renderPass, uint32_t windowWidth, uint32_t windowHeight);
	static void StaticFinalize();

	static void PreDraw(CommandList cmd);

	static std::unique_ptr<Sprite> Create(
		uint32_t textureHandle,
		Vector2 position,
		Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f},
		Vector2 anchorPoint = {0.0f, 0.0f},
		bool isFlipX = false,
		bool isFlipY = false);


private: // 静的メンバ変数

	static constexpr uint32_t kNumVertex = 4;

	static inline Shader sVs_{};
	static inline Shader sPs_{};
	static inline PipelineState sPso_{};

	static inline Matrix4x4 sMapProj_{};


public: // メンバ関数

	void Draw(CommandList cmd) const;

	void SetTextureHandle(uint32_t textureHandle);
	uint32_t GetTextureHandle() const;

	void SetTranslation(const Vector2 &translation);
	const Vector2 &GetTranslation() const;

	void SetRotation(const Vector3 &rotation);
	const Vector3 &GetRotation() const;

	void SetScale(const Vector2 &scale);
	const Vector2 &GetScale() const;

	void SetUvTranslation(const Vector2 &uvTranslation);
	const Vector2 &GetUvTranslation() const;

	void SetUvRotation(float uvRotation);
	float GetUvRotation() const;

	void SetUvScale(const Vector2 &uvScale);
	const Vector2 &GetUvScale() const;

	void SetSize(const Vector2 &size);
	const Vector2 &GetSize() const;

	void SetAnchorPoint(const Vector2 &anchorPoint);
	const Vector2 &GetAnchorPoint() const;

	void SetColor(const Vector4 &color);
	const Vector4 &GetColor() const;

	void SetIsFlipX(bool isFlipX);
	bool IsFlipX() const;

	void SetIsFlipY(bool isFlipY);
	bool IsFlipY() const;

	void SetTextureRect(const Vector2 &texBase, const Vector2 &texSize);


private: // メンバ変数

	uint32_t textureHandle_ = 0;

	Vector2 translation_{};
	Vector3 rotation_{};
	Vector2 scale_ = {1.0f, 1.0f};

	Vector2 uvTranslation_{};
	float uvRotation_{};
	Vector2 uvScale_ = {1.0f, 1.0f};

	Vector2 size_{};
	Vector2 anchorPoint_{};
	Vector4 color_{};

	bool isFlipX_ = false;
	bool isFlipY_ = false;

	Vector2 texBase_{};
	Vector2 texSize_{};


private: // コンストラクタ

	Sprite(
		uint32_t textureHandle,
		const Vector2 &position,
		const Vector4 &color,
		const Vector2 &anchorPoint,
		bool isFlipX,
		bool isFlipY);
};

} // namespace ou
