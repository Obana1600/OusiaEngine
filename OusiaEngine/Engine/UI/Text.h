#pragma once

#include "Graphics/GraphicsDevice.h"
#include "Math/Color.h"
#include "Math/Matrix4x4.h"
#include "Math/Rect.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "UI/Graphic.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>


namespace ou {

// 前方宣言
class FontAsset;

enum class TextAlignment : uint8_t {
	TopLeft,
	Top,
	TopRight,
	Left,
	Center,
	Right,
	BottomLeft,
	Bottom,
	BottomRight,
};

class Text final: public Graphic {
public: // インナークラス

	// 定数バッファ: b0
	struct PerText {
		Matrix4x4 worldViewProj;
		Color color;
		Color outlineColor;
		Color underlayColor;
		float faceDilate;
		float outlineWidth;
		float padding[2];
		float underlayDilate;
		float underlaySoftness;
		Vector2 underlayUvOffset;
		uint32_t glyphBufferIndex;
	};

	// 1 文字の矩形
	struct GlyphQuad {
		Vector4 positionRect;
		Vector4 texcoordRect;
		uint32_t textureIndex;
		uint32_t padding[3];
	};


public: // 静的メンバ関数

	static void StaticInitialize();
	static void StaticFinalize();


public: // メンバ関数

	void LateUpdate() override;

	void Draw(Graphics::CommandList cmd, const Matrix4x4 &viewProj) const override;

	const std::shared_ptr<Material> &GetDefaultMaterial() const override;

	void SetText(std::string_view text);
	const std::string &GetText() const;

	void SetFont(std::shared_ptr<FontAsset> font);
	const std::shared_ptr<FontAsset> &GetFont() const;

	void SetFontSize(float fontSize);
	float GetFontSize() const;

	void SetAlignment(TextAlignment alignment);
	TextAlignment GetAlignment() const;

	void SetWordWrappingEnabled(bool wordWrappingEnabled);
	bool IsWordWrappingEnabled() const;

	void SetLineSpacing(float lineSpacing);
	float GetLineSpacing() const;

	void SetCharacterSpacing(float characterSpacing);
	float GetCharacterSpacing() const;

	void SetFaceDilate(float faceDilate);
	float GetFaceDilate() const;

	void SetOutlineColor(const Color &outlineColor);
	const Color &GetOutlineColor() const;

	void SetOutlineWidth(float outlineWidth);
	float GetOutlineWidth() const;

	void SetUnderlayColor(const Color &underlayColor);
	const Color &GetUnderlayColor() const;

	void SetUnderlayOffset(const Vector2 &underlayOffset);
	const Vector2 &GetUnderlayOffset() const;

	void SetUnderlayDilate(float underlayDilate);
	float GetUnderlayDilate() const;

	void SetUnderlaySoftness(float underlaySoftness);
	float GetUnderlaySoftness() const;


private: // 静的メンバ変数

	static inline std::shared_ptr<Material> sDefaultMaterial_;
	static inline std::shared_ptr<FontAsset> sDefaultFont_;


private: // インナークラス

	struct PlacedGlyph {
		Vector4 positionRect;
		Vector4 texcoordRect;
		uint32_t atlasIndex;
	};


private: // メンバ変数

	std::string text_;
	std::shared_ptr<FontAsset> font_;
	float fontSize_ = 36.0f;
	TextAlignment alignment_ = TextAlignment::TopLeft;
	bool wordWrappingEnabled_ = true;
	float lineSpacing_ = 0.0f;
	float characterSpacing_ = 0.0f;

	float faceDilate_ = 0.0f;
	float outlineWidth_ = 0.0f;
	Color outlineColor_ = Color::Black;
	float underlayDilate_ = 0.0f;
	float underlaySoftness_ = 0.0f;
	Vector2 underlayOffset_{};
	Color underlayColor_ = Color::Clear;

	std::vector<PlacedGlyph> placedGlyphs_;
	Rect layoutRect_{};
	bool isLayoutDirty_ = true;

	Graphics::GpuBuffer glyphBuffers_[Graphics::GraphicsDevice::BUFFER_COUNT];
	uint32_t capacity_ = 0;
	uint32_t quadCount_ = 0;


private: // メンバ関数

	const std::shared_ptr<FontAsset> &FindFont() const;

	void RebuildLayout(FontAsset &font);
	void EnsureCapacity(uint32_t count);
};

} // namespace ou
