#include "Text.h"
#include "Asset/AssetDatabase.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Fatal.h"
#include "Math/Mathf.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Shader.h"
#include "Render/Asset/Texture.h"
#include "Serialize/Serializer.h"
#include "UI/Asset/FontAsset.h"
#include "UI/CanvasRenderer.h"
#include "UI/RectTransform.h"
#include "UI/VertexHelper.h"

#include <algorithm>
#include <utility>

using namespace ou;


namespace {

// フォントにない文字の代わりに出す文字
constexpr char32_t kMissingCharacter = U'\u25A1';

// 並べる途中の 1 文字
struct LineGlyph {
	const Glyph *glyph;
	float x;
	float advance;
	bool isWhitespace;
};

// 行末の空白を数えない幅
float MeasureLineWidth(const std::vector<LineGlyph> &line) {
	for (auto it = line.rbegin(); it != line.rend(); it++) {
		if (!it->isWhitespace) { return it->x + it->advance; }
	}
	return 0.0f;
}

} // namespace



void ou::Text::StaticInitialize() {
	const auto shader = AssetDatabase::LoadAssetAtPath<Shader>("EngineAssets/Shader/Text.shader");
	if (!shader) {
		FatalError("テキストのシェーダの読み込みに失敗しました");
	}
	sDefaultMaterial_ = Material::Create(shader);
	sDefaultMaterial_->SetName("Default Text Material");

	sDefaultFont_ = AssetDatabase::LoadAssetAtPath<FontAsset>("EngineAssets/Font/NotoSansJP-Regular.ttf");
	if (!sDefaultFont_) {
		FatalError("テキストの既定のフォントの読み込みに失敗しました");
	}
}


void ou::Text::StaticFinalize() {
	sDefaultMaterial_ = nullptr;
	sDefaultFont_ = nullptr;
}


void ou::Text::LateUpdate() {
	if (IsAtlasTextureChanged()) {
		SetMaterialDirty();
	}
}


void ou::Text::Serialize(Serializer &serializer) {
	Graphic::Serialize(serializer);

	serializer.Field("text", text_);
	serializer.Asset("font", font_);
	serializer.Field("fontSize", fontSize_);
	serializer.Enum("alignment", alignment_);
	serializer.Field("wordWrappingEnabled", wordWrappingEnabled_);
	serializer.Field("lineSpacing", lineSpacing_);
	serializer.Field("characterSpacing", characterSpacing_);
	serializer.Field("faceDilate", faceDilate_);
	serializer.Field("outlineWidth", outlineWidth_);
	serializer.Field("outlineColor", outlineColor_);
	serializer.Field("underlayDilate", underlayDilate_);
	serializer.Field("underlaySoftness", underlaySoftness_);
	serializer.Field("underlayOffset", underlayOffset_);
	serializer.Field("underlayColor", underlayColor_);

	if (serializer.IsReading()) {
		SetAllDirty();
	}
}


const std::shared_ptr<Material> &ou::Text::GetDefaultMaterial() const {
	return sDefaultMaterial_;
}


void ou::Text::SetText(std::string_view text) {
	if (text_ == text) { return; }
	text_ = text;
	SetVerticesDirty();
}


const std::string &ou::Text::GetText() const {
	return text_;
}


void ou::Text::SetFont(std::shared_ptr<FontAsset> font) {
	font_ = std::move(font);
	SetAllDirty();
}


const std::shared_ptr<FontAsset> &ou::Text::GetFont() const {
	return font_;
}


void ou::Text::SetFontSize(float fontSize) {
	fontSize_ = fontSize;
	SetVerticesDirty();
}


float ou::Text::GetFontSize() const {
	return fontSize_;
}


void ou::Text::SetAlignment(TextAlignment alignment) {
	alignment_ = alignment;
	SetVerticesDirty();
}


TextAlignment ou::Text::GetAlignment() const {
	return alignment_;
}


void ou::Text::SetWordWrappingEnabled(bool wordWrappingEnabled) {
	wordWrappingEnabled_ = wordWrappingEnabled;
	SetVerticesDirty();
}


bool ou::Text::IsWordWrappingEnabled() const {
	return wordWrappingEnabled_;
}


void ou::Text::SetLineSpacing(float lineSpacing) {
	lineSpacing_ = lineSpacing;
	SetVerticesDirty();
}


float ou::Text::GetLineSpacing() const {
	return lineSpacing_;
}


void ou::Text::SetCharacterSpacing(float characterSpacing) {
	characterSpacing_ = characterSpacing;
	SetVerticesDirty();
}


float ou::Text::GetCharacterSpacing() const {
	return characterSpacing_;
}


void ou::Text::SetFaceDilate(float faceDilate) {
	faceDilate_ = Mathf::ClampN1P1(faceDilate);
	SetMaterialDirty();
}


float ou::Text::GetFaceDilate() const {
	return faceDilate_;
}


void ou::Text::SetOutlineWidth(float outlineWidth) {
	outlineWidth_ = Mathf::Clamp01(outlineWidth);
	SetMaterialDirty();
}


float ou::Text::GetOutlineWidth() const {
	return outlineWidth_;
}


void ou::Text::SetOutlineColor(const Color &outlineColor) {
	outlineColor_ = outlineColor;
	SetMaterialDirty();
}


const Color &ou::Text::GetOutlineColor() const {
	return outlineColor_;
}


void ou::Text::SetUnderlayDilate(float underlayDilate) {
	underlayDilate_ = Mathf::ClampN1P1(underlayDilate);
	SetMaterialDirty();
}


float ou::Text::GetUnderlayDilate() const {
	return underlayDilate_;
}


void ou::Text::SetUnderlaySoftness(float underlaySoftness) {
	underlaySoftness_ = Mathf::Clamp01(underlaySoftness);
	SetMaterialDirty();
}


float ou::Text::GetUnderlaySoftness() const {
	return underlaySoftness_;
}


void ou::Text::SetUnderlayOffset(const Vector2 &underlayOffset) {
	underlayOffset_ = {Mathf::ClampN1P1(underlayOffset.x),
					   Mathf::ClampN1P1(underlayOffset.y)};
	SetMaterialDirty();
}


const Vector2 &ou::Text::GetUnderlayOffset() const {
	return underlayOffset_;
}


void ou::Text::SetUnderlayColor(const Color &underlayColor) {
	underlayColor_ = underlayColor;
	SetMaterialDirty();
}


const Color &ou::Text::GetUnderlayColor() const {
	return underlayColor_;
}


void ou::Text::UpdateGeometry() {
	auto *canvasRenderer = GetCanvasRenderer();
	const auto &font = FindFont();
	if (!font) {
		canvasRenderer->Clear();
		return;
	}

	RebuildLayout(*font);

	font->UpdateAtlasTextures();

	std::ranges::stable_sort(placedGlyphs_, {}, &PlacedGlyph::atlasIndex);

	VertexHelper vh;
	std::vector<CanvasRenderer::SubMesh> subMeshes(font->GetAtlasTextureCount());
	const auto &color = GetColor();
	for (const auto &placed : placedGlyphs_) {
		const auto &position = placed.positionRect;
		const auto &texcoord = placed.texcoordRect;
		const UIVertex quad[4] = {
			{{position.x, position.w, 0.0f}, color, {texcoord.x, texcoord.w, 0.0f, 0.0f}, texcoord},
			{{position.x, position.y, 0.0f}, color, {texcoord.x, texcoord.y, 0.0f, 0.0f}, texcoord},
			{{position.z, position.y, 0.0f}, color, {texcoord.z, texcoord.y, 0.0f, 0.0f}, texcoord},
			{{position.z, position.w, 0.0f}, color, {texcoord.z, texcoord.w, 0.0f, 0.0f}, texcoord}};
		vh.AddUIVertexQuad(quad);
		subMeshes[placed.atlasIndex].indexCount += 6;
	}
	for (size_t i = 1; i < subMeshes.size(); i++) {
		subMeshes[i].indexStart = subMeshes[i - 1].indexStart + subMeshes[i - 1].indexCount;
	}

	std::vector<UIVertex> vertices;
	std::vector<uint32_t> indices;
	vh.FillMesh(vertices, indices);
	canvasRenderer->SetMesh(std::move(vertices), std::move(indices), std::move(subMeshes));

	if (IsAtlasTextureChanged()) {
		UpdateMaterial();
	}
}


void ou::Text::UpdateMaterial() {
	auto *canvasRenderer = GetCanvasRenderer();
	const auto &font = FindFont();
	if (!font) { return; }

	const auto uvPerOffset = static_cast<float>(font->GetPadding()) / static_cast<float>(font->GetAtlasSize());
	const Vector4 underlayUvOffset = {-underlayOffset_.x * uvPerOffset,
									  underlayOffset_.y * uvPerOffset,
									  0.0f, 0.0f};

	const auto pageCount = font->GetAtlasTextureCount();
	pageMaterials_.resize(pageCount);
	canvasRenderer->SetMaterialCount(pageCount);
	canvasRenderer->SetTexture(nullptr);

	for (uint32_t i = 0; i < pageCount; i++) {
		auto &material = pageMaterials_[i];
		material = Material::Create(*GetMaterial());
		material->SetMainTexture(font->GetAtlasTexture(i));

		material->SetMainTexture(font->GetAtlasTexture(i));
		material->SetColor("_OutlineColor", outlineColor_);
		material->SetColor("_UnderlayColor", underlayColor_);
		material->SetVector("_UnderlayUvOffset", underlayUvOffset);
		material->SetFloat("_FaceDilate", faceDilate_);
		material->SetFloat("_OutlineWidth", outlineWidth_);
		material->SetFloat("_UnderlayDilate", underlayDilate_);
		material->SetFloat("_UnderlaySoftness", underlaySoftness_);
		canvasRenderer->SetMaterial(material, i);
	}
}


const std::shared_ptr<FontAsset> &ou::Text::FindFont() const {
	return font_ ? font_ : sDefaultFont_;
}


void ou::Text::RebuildLayout(FontAsset &font) {
	placedGlyphs_.clear();

	// フォントのサイズや間隔を計算
	const auto &face = font.GetFaceInfo();
	const auto &rect = GetRectTransform()->GetRect();
	const auto scale = fontSize_ / face.pointSize;
	const auto spacing = characterSpacing_ * fontSize_ * 0.01f;
	const auto lineHeight = face.lineHeight * scale + lineSpacing_ * fontSize_ * 0.01f;

	std::vector<std::vector<LineGlyph>> lines(1);
	float penX = 0.0f;
	size_t breakIndex = SIZE_MAX;

	for (const auto unicode : Utf8ToUtf32(text_)) {
		// 改行処理
		if (unicode == U'\n') {
			lines.emplace_back();
			penX = 0.0f;
			breakIndex = SIZE_MAX;
			continue;
		}

		// グリフを取得しなければ代替文字を使う
		const auto *glyph = font.FindOrAddGlyph(unicode);
		if (!glyph) { glyph = font.FindOrAddGlyph(kMissingCharacter); }
		if (!glyph) { continue; }

		const auto advance = glyph->metrics.horizontalAdvance * scale + spacing;

		// 幅を超えたら折り返す
		auto &line = lines.back();
		if (wordWrappingEnabled_ && !line.empty() && penX + advance > rect.width) {
			std::vector<LineGlyph> next;
			if (breakIndex != SIZE_MAX) {
				next.assign(line.begin() + static_cast<ptrdiff_t>(breakIndex) + 1, line.end());
				line.resize(breakIndex);

				const auto shift = next.empty() ? penX : next.front().x;
				for (auto &moved : next) {
					moved.x -= shift;
				}
				penX -= shift;
			} else {
				penX = 0.0f;
			}
			lines.push_back(std::move(next));
			breakIndex = SIZE_MAX;
		}

		// 次の文字を置く位置を進める
		auto &current = lines.back();
		const bool isWhitespace = (unicode == U' ' || unicode == U'\u3000');
		if (isWhitespace) { breakIndex = current.size(); }
		current.push_back({glyph, penX, advance, isWhitespace});
		penX += advance;
	}

	// アライメント
	const auto horizontal = static_cast<uint32_t>(alignment_) % 3;
	const auto vertical = static_cast<uint32_t>(alignment_) / 3;

	// テキスト全体の高さと矩形の上端の計算
	const auto ascent = face.ascentLine * scale;
	const auto descent = face.descentLine * scale;
	const auto blockHeight = ascent - descent + lineHeight * static_cast<float>(lines.size() - 1);
	const auto rectTop = rect.GetYMax();

	// 垂直アライメント
	auto blockTop = rectTop;
	if (vertical == 1) {
		blockTop = rectTop - (rect.height - blockHeight) * 0.5f;
	} else if (vertical == 2) {
		blockTop = rect.GetYMin() + blockHeight;
	}
	const auto firstBaseline = blockTop - ascent;

	const auto atlasSize = static_cast<float>(font.GetAtlasSize());
	for (size_t i = 0; i < lines.size(); i++) {
		// 行のベースラインと横幅
		const auto &line = lines[i];
		const auto baseline = firstBaseline - lineHeight * static_cast<float>(i);
		const auto lineWidth = MeasureLineWidth(line);

		// 水平アライメント
		auto lineLeft = rect.GetXMin();
		if (horizontal == 1) {
			lineLeft += (rect.width - lineWidth) * 0.5f;
		} else if (horizontal == 2) {
			lineLeft += rect.width - lineWidth;
		}

		for (const auto &lineGlyph : line) {
			const auto &glyphRect = lineGlyph.glyph->glyphRect;
			if (glyphRect.width <= 0.0f) { continue; }

			// 描画する矩形の左上座標
			const auto &offset = lineGlyph.glyph->bitmapOffset;
			const auto left = lineLeft + lineGlyph.x + offset.x * scale;
			const auto top = baseline + offset.y * scale;

			// 描画する矩形を保存
			PlacedGlyph placed{};
			placed.positionRect = {
				left,
				top,
				left + glyphRect.width * scale,
				top - glyphRect.height * scale};
			placed.texcoordRect = {
				glyphRect.x / atlasSize,
				glyphRect.y / atlasSize,
				glyphRect.GetXMax() / atlasSize,
				glyphRect.GetYMax() / atlasSize};
			placed.atlasIndex = lineGlyph.glyph->atlasIndex;
			placedGlyphs_.push_back(placed);
		}
	}
}


bool ou::Text::IsAtlasTextureChanged() const {
	const auto &font = FindFont();
	if (!font) { return false; }

	const auto pageCount = font->GetAtlasTextureCount();
	if (pageMaterials_.size() != pageCount) { return true; }

	for (uint32_t i = 0; i < pageCount; i++) {
		if (pageMaterials_[i]->GetMainTexture() != font->GetAtlasTexture(i)) { return true; }
	}

	return false;
}
