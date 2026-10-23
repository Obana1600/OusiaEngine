#include "FontAsset.h"
#include "Diagnostics/Log.h"
#include "Graphics/GraphicsDevice.h"
#include "Render/Asset/Texture.h"

#include <algorithm>
#include <cstring>
#include <format>
#include <utility>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H

using namespace ou;


namespace {

// SDF の広がりの範囲
constexpr uint32_t kMinSpread = 2;
constexpr uint32_t kMaxSpread = 32;

// アトラスの 1 ページの大きさの上限
constexpr uint32_t kMaxAtlasSize = 16384;

// 隣の文字やページの端がにじまないように空ける幅
constexpr uint32_t kGlyphSpacing = 1;

// 26.6 固定小数点 -> ピクセル
float FromFixed26Dot6(FT_Pos value) {
	return static_cast<float>(value) / 64.0f;
}

} // namespace



std::shared_ptr<FontAsset> ou::FontAsset::Create(
	std::vector<uint8_t> fontData, uint32_t samplingPointSize, uint32_t padding, uint32_t atlasSize) {
	if (padding < kMinSpread || padding > kMaxSpread) {
		Log::Error("フォントの padding は {}~{} にしてください: {}", kMinSpread, kMaxSpread, padding);
		return nullptr;
	}
	if (atlasSize == 0 || atlasSize > kMaxAtlasSize) {
		Log::Error("アトラスの大きさは 1~{} にしてください: {}", kMaxAtlasSize, atlasSize);
		return nullptr;
	}

	auto font = std::shared_ptr<FontAsset>(new FontAsset());
	font->fontData_ = std::move(fontData);
	font->samplingPointSize_ = samplingPointSize;
	font->padding_ = padding;
	font->atlasSize_ = atlasSize;

	// Library の初期化
	auto error = FT_Init_FreeType(&font->library_);
	if (error) {
		Log::Error("FreeType の初期化に失敗しました (error = {})", error);
		return nullptr;
	}

	// SDF の広がりの設定
	const auto spread = static_cast<FT_Int>(padding);
	error = FT_Property_Set(font->library_, "bsdf", "spread", &spread);
	if (error) {
		Log::Error("SDF の広がりの設定に失敗しました (error = {})", error);
		return nullptr;
	}

	// フォントの読み込み
	error = FT_New_Memory_Face(
		font->library_, font->fontData_.data(), static_cast<FT_Long>(font->fontData_.size()), 0, &font->face_);
	if (error) {
		Log::Error("フォントの読み込みに失敗しました (error = {})", error);
		return nullptr;
	}

	// フォントサイズの設定
	error = FT_Set_Pixel_Sizes(font->face_, 0, samplingPointSize);
	if (error) {
		Log::Error("フォントサイズの設定に失敗しました: {} (error = {})", samplingPointSize, error);
		return nullptr;
	}

	// フォント全体の寸法を取り出す
	const auto &metrics = font->face_->size->metrics;
	font->faceInfo_.pointSize = static_cast<float>(samplingPointSize);
	font->faceInfo_.lineHeight = FromFixed26Dot6(metrics.height);
	font->faceInfo_.ascentLine = FromFixed26Dot6(metrics.ascender);
	font->faceInfo_.descentLine = FromFixed26Dot6(metrics.descender);

	return font;
}


ou::FontAsset::~FontAsset() {
	if (face_) {
		FT_Done_Face(face_);
	}
	if (library_) {
		FT_Done_FreeType(library_);
	}
}


const Glyph *ou::FontAsset::FindOrAddGlyph(char32_t unicode) {
	// 登録済みの文字か
	if (const auto it = glyphs_.find(unicode); it != glyphs_.end()) {
		return &it->second;
	}

	// 以前失敗した文字か
	if (missingCharacters_.contains(unicode)) { return nullptr; }

	// 文字の画像を作成する
	const auto bitmap = RenderGlyph(unicode);
	if (!bitmap) {
		missingCharacters_.insert(unicode);
		return nullptr;
	}

	// 文字の配置情報を取り出す
	Glyph glyph;
	glyph.metrics = bitmap->metrics;
	glyph.bitmapOffset = {static_cast<float>(bitmap->left),
						  static_cast<float>(bitmap->top)};

	// 輪郭のある文字だけアトラスに追記する
	if (!bitmap->pixels.empty()) {
		if (!ReserveAtlasRect(bitmap->width, bitmap->height, &glyph.atlasIndex, &glyph.glyphRect)) {
			Log::Error("文字がアトラスのページに入りません: U+{:04X} ({}x{}, ページ {}x{})",
					   static_cast<uint32_t>(unicode), bitmap->width, bitmap->height, atlasSize_, atlasSize_);
			missingCharacters_.insert(unicode);
			return nullptr;
		}

		// ページの画像に写す
		auto &page = atlasPages_[glyph.atlasIndex];
		const auto x0 = static_cast<size_t>(glyph.glyphRect.position.x);
		const auto y0 = static_cast<size_t>(glyph.glyphRect.position.y);
		for (uint32_t y = 0; y < bitmap->height; y++) {
			std::memcpy(page.pixels.data() + (y0 + y) * atlasSize_ + x0,
						bitmap->pixels.data() + static_cast<size_t>(y) * bitmap->width,
						bitmap->width);
		}
		page.isDirty = true;
	}

	// 文字を登録して返す
	const auto [it, inserted] = glyphs_.emplace(unicode, glyph);
	return &it->second;
}


void ou::FontAsset::UpdateAtlasTextures() {
	auto *device = Graphics::GetDevice();

	for (size_t i = 0; i < atlasPages_.size(); i++) {
		auto &page = atlasPages_[i];
		if (!page.isDirty) { continue; }
		page.isDirty = false;

		// グラフィクス層のテクスチャの設定
		Graphics::TextureDesc desc{};
		desc.width = atlasSize_;
		desc.height = atlasSize_;
		desc.arraySize = 1;
		desc.mipLevels = 1;
		desc.format = Graphics::Format::R8_UNORM;
		desc.bindFlags = Graphics::BindFlag::SHADER_RESOURCE;
		desc.layout = Graphics::ResourceState::SHADER_RESOURCE;

		// テクスチャのデータを渡す
		Graphics::SubresourceData data{};
		data.dataPtr = page.pixels.data();
		data.rowPitch = atlasSize_;
		data.slicePitch = atlasSize_ * atlasSize_;

		// ページごと作り直す
		Graphics::Texture graphicsTexture;
		if (!device->CreateTexture(&desc, &data, &graphicsTexture)) {
			Log::Error("フォントアトラスの作成に失敗しました: {} (ページ {})", GetName(), i);
			continue;
		}
		const auto name = std::format("{} Atlas {}", GetName(), i);
		device->SetName(&graphicsTexture, name);

		page.texture = std::make_shared<Texture>(std::move(graphicsTexture));
		page.texture->SetName(name);
	}
}


const FaceInfo &ou::FontAsset::GetFaceInfo() const {
	return faceInfo_;
}


uint32_t ou::FontAsset::GetSamplingPointSize() const {
	return samplingPointSize_;
}


uint32_t ou::FontAsset::GetPadding() const {
	return padding_;
}


uint32_t ou::FontAsset::GetAtlasSize() const {
	return atlasSize_;
}


uint32_t ou::FontAsset::GetAtlasTextureCount() const {
	return static_cast<uint32_t>(atlasPages_.size());
}


std::shared_ptr<Texture> ou::FontAsset::GetAtlasTexture(uint32_t atlasIndex) const {
	if (atlasIndex >= atlasPages_.size()) { return nullptr; }
	return atlasPages_[atlasIndex].texture;
}


std::optional<GlyphBitmap> ou::FontAsset::RenderGlyph(char32_t unicode) const {
	// フォントにない文字
	const auto glyphIndex = FT_Get_Char_Index(face_, unicode);
	if (glyphIndex == 0) { return std::nullopt; }

	// 文字の読み込み
	auto error = FT_Load_Glyph(face_, glyphIndex, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
	if (error) {
		Log::Error("文字の読み込みに失敗しました: U+{:04X} (error = {})", static_cast<uint32_t>(unicode), error);
		return std::nullopt;
	}

	// 文字の寸法を取り出す
	auto *slot = face_->glyph;
	GlyphBitmap glyph;
	glyph.metrics.width = FromFixed26Dot6(slot->metrics.width);
	glyph.metrics.height = FromFixed26Dot6(slot->metrics.height);
	glyph.metrics.horizontalBearingX = FromFixed26Dot6(slot->metrics.horiBearingX);
	glyph.metrics.horizontalBearingY = FromFixed26Dot6(slot->metrics.horiBearingY);
	glyph.metrics.horizontalAdvance = FromFixed26Dot6(slot->metrics.horiAdvance);

	// 輪郭のない文字
	if (slot->outline.n_points == 0) { return glyph; }

	// SDF 変換
	error = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
	if (!error) {
		error = FT_Render_Glyph(slot, FT_RENDER_MODE_SDF);
		if (error) {
			Log::Error("文字の SDF 変換に失敗しました: U+{:04X} (error = {})", static_cast<uint32_t>(unicode), error);
			return std::nullopt;
		}
	}

	// 1 画素 1 バイトの形式で出力されているか
	const auto &bitmap = slot->bitmap;
	if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
		Log::Error("SDF の画素の形式が想定と違います: U+{:04X} (pixel_mode = {})",
				   static_cast<uint32_t>(unicode), bitmap.pixel_mode);
		return std::nullopt;
	}

	// SDF の情報を取り出す
	glyph.left = slot->bitmap_left;
	glyph.top = slot->bitmap_top;
	glyph.width = bitmap.width;
	glyph.height = bitmap.rows;
	glyph.pixels.resize(static_cast<size_t>(glyph.width) * glyph.height);
	for (uint32_t y = 0; y < glyph.height; y++) {
		std::memcpy(glyph.pixels.data() + static_cast<size_t>(y) * glyph.width,
					bitmap.buffer + static_cast<ptrdiff_t>(y) * bitmap.pitch,
					glyph.width);
	}

	return glyph;
}


bool ou::FontAsset::ReserveAtlasRect(uint32_t width, uint32_t height, uint32_t *outAtlasIndex, Rect *outRect) {
	// 1 ページに収まる大きさか
	if (width + kGlyphSpacing * 2 > atlasSize_ || height + kGlyphSpacing * 2 > atlasSize_) {
		return false;
	}

	// ページがなければ追加する
	if (atlasPages_.empty()) {
		AddAtlasPage();
	}
	auto *page = &atlasPages_.back();

	// 今の行に入らなければ次の行へ移動
	if (page->cursorX + width + kGlyphSpacing > atlasSize_) {
		page->cursorX = kGlyphSpacing;
		page->cursorY += page->shelfHeight;
		page->shelfHeight = 0;
	}

	// 今のページの最後に入らなければ新しいページを追加
	if (page->cursorY + height + kGlyphSpacing > atlasSize_) {
		AddAtlasPage();
		page = &atlasPages_.back();
	}

	// 呼び出し元に返す
	*outAtlasIndex = static_cast<uint32_t>(atlasPages_.size() - 1);
	*outRect = {{static_cast<float>(page->cursorX), static_cast<float>(page->cursorY)},
				{static_cast<float>(width), static_cast<float>(height)}};

	// 次の画像用にカーソルを進める
	page->cursorX += width + kGlyphSpacing;
	page->shelfHeight = std::max(page->shelfHeight, height + kGlyphSpacing);
	return true;
}


void ou::FontAsset::AddAtlasPage() {
	AtlasPage page;
	page.pixels.assign(static_cast<size_t>(atlasSize_) * atlasSize_, 0);
	page.cursorX = kGlyphSpacing;
	page.cursorY = kGlyphSpacing;
	atlasPages_.push_back(std::move(page));
}
