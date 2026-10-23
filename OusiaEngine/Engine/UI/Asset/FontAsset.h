#pragma once

#include "Core/Object.h"
#include "Math/Rect.h"
#include "Math/Vector2.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// 前方宣言
struct FT_LibraryRec_;
struct FT_FaceRec_;


namespace ou {

// 前方宣言
class Texture;

// フォント全体の寸法
struct FaceInfo {
	float pointSize = 0.0f;	  // サンプリングのサイズ
	float lineHeight = 0.0f;  // 行の高さ
	float ascentLine = 0.0f;  // ベースラインから上端まで
	float descentLine = 0.0f; // ベースラインから下端まで
};

// 1 文字の寸法
struct GlyphMetrics {
	float width = 0.0f;
	float height = 0.0f;
	float horizontalBearingX = 0.0f; // 原点から文字の左端まで
	float horizontalBearingY = 0.0f; // ベースラインから文字の上端まで
	float horizontalAdvance = 0.0f;	 // 次の文字の原点まで
};

// 1 文字の SDF の画像
struct GlyphBitmap {
	GlyphMetrics metrics;
	int32_t left = 0;
	int32_t top = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	std::vector<uint8_t> pixels;
};

// アトラスに入れる 1 文字
struct Glyph {
	GlyphMetrics metrics;
	Rect glyphRect{};		 // アトラス上の位置
	Vector2 bitmapOffset{};	 // 原点から画像の左上まで
	uint32_t atlasIndex = 0; // どのページか
};

class FontAsset final: public Object {
private: // パスキー

	struct Passkey {
		explicit Passkey() = default;
	};


public: // 静的メンバ関数

	static std::shared_ptr<FontAsset> Create(std::vector<uint8_t> fontData,
											 uint32_t samplingPointSize,
											 uint32_t padding,
											 uint32_t atlasSize);


public: // メンバ関数

	explicit FontAsset(Passkey) {}
	~FontAsset() override;

	const Glyph *FindOrAddGlyph(char32_t unicode);
	void UpdateAtlasTextures();

	const FaceInfo &GetFaceInfo() const;
	uint32_t GetSamplingPointSize() const;
	uint32_t GetPadding() const;

	uint32_t GetAtlasSize() const;
	uint32_t GetAtlasTextureCount() const;
	std::shared_ptr<Texture> GetAtlasTexture(uint32_t atlasIndex) const;


private: // インナークラス

	struct AtlasPage {
		std::vector<uint8_t> pixels;
		std::shared_ptr<Texture> texture;
		uint32_t cursorX = 0;
		uint32_t cursorY = 0;
		uint32_t shelfHeight = 0;
		bool isDirty = false;
	};


private: // メンバ変数

	std::vector<uint8_t> fontData_;
	FT_LibraryRec_ *library_ = nullptr;
	FT_FaceRec_ *face_ = nullptr;

	FaceInfo faceInfo_{};
	uint32_t samplingPointSize_ = 0;
	uint32_t padding_ = 0;

	uint32_t atlasSize_ = 0;
	std::vector<AtlasPage> atlasPages_;
	std::unordered_map<char32_t, Glyph> glyphs_;
	std::unordered_set<char32_t> missingCharacters_;


private: // メンバ関数

	std::optional<GlyphBitmap> RenderGlyph(char32_t unicode) const;

	bool ReserveAtlasRect(uint32_t width, uint32_t height, uint32_t *outAtlasIndex, Rect *outRect);
	void AddAtlasPage();
};

} // namespace ou
