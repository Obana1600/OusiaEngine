#include "Text.h"
#include "Asset/AssetDatabase.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Fatal.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Shader.h"
#include "Render/Asset/Texture.h"
#include "UI/FontAsset.h"
#include "UI/RectTransform.h"

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
	// フォントの取得
	const auto &font = FindFont();
	if (!font) {
		quadCount_ = 0;
		return;
	}

	// 描画位置を再計算する必要があるか
	const auto &rect = GetRectTransform()->GetRect();
	if (!(rect.position == layoutRect_.position) || !(rect.size == layoutRect_.size)) {
		isLayoutDirty_ = true;
	}
	if (isLayoutDirty_) {
		RebuildLayout(*font);
		layoutRect_ = rect;
		isLayoutDirty_ = false;
	}

	font->UpdateAtlasTextures();

	// バッファの容量の確保
	quadCount_ = 0;
	if (placedGlyphs_.empty()) { return; }
	EnsureCapacity(static_cast<uint32_t>(placedGlyphs_.size()));

	// 各文字のデータをバッファに書き込む
	auto &buffer = glyphBuffers_[Graphics::GetDevice()->GetBufferIndex()];
	auto *data = static_cast<GlyphQuad *>(buffer.mappedData);
	for (const auto &placed : placedGlyphs_) {
		const auto texture = font->GetAtlasTexture(placed.atlasIndex);
		if (!texture) { continue; }

		auto &quad = data[quadCount_++];
		quad.positionRect = placed.positionRect;
		quad.texcoordRect = placed.texcoordRect;
		quad.textureIndex = texture->GetDescriptorIndex();
	}
}


void ou::Text::Draw(Graphics::CommandList cmd, const Matrix4x4 &viewProj) const {
	if (quadCount_ == 0) { return; }

	auto *device = Graphics::GetDevice();
	const auto &buffer = glyphBuffers_[device->GetBufferIndex()];

	PerText constants{};
	constants.worldViewProj = GetRectTransform()->GetLocalToWorldMatrix() * viewProj;
	constants.color = GetColor().Linear();
	constants.glyphBufferIndex =
		static_cast<uint32_t>(device->GetDescriptorIndex(&buffer, Graphics::ViewType::SRV));

	device->BindDynamicConstantBuffer(constants, 0, cmd);
	DrawWithMaterial(cmd, 4, quadCount_);
}


const std::shared_ptr<Material> &ou::Text::GetDefaultMaterial() const {
	return sDefaultMaterial_;
}


void ou::Text::SetText(std::string_view text) {
	if (text_ == text) { return; }
	text_ = text;
	isLayoutDirty_ = true;
}


const std::string &ou::Text::GetText() const {
	return text_;
}


void ou::Text::SetFont(std::shared_ptr<FontAsset> font) {
	font_ = std::move(font);
	isLayoutDirty_ = true;
}


const std::shared_ptr<FontAsset> &ou::Text::GetFont() const {
	return font_;
}


void ou::Text::SetFontSize(float fontSize) {
	fontSize_ = fontSize;
	isLayoutDirty_ = true;
}


float ou::Text::GetFontSize() const {
	return fontSize_;
}


void ou::Text::SetAlignment(TextAlignment alignment) {
	alignment_ = alignment;
	isLayoutDirty_ = true;
}


TextAlignment ou::Text::GetAlignment() const {
	return alignment_;
}


void ou::Text::SetWordWrappingEnabled(bool wordWrappingEnabled) {
	wordWrappingEnabled_ = wordWrappingEnabled;
	isLayoutDirty_ = true;
}


bool ou::Text::IsWordWrappingEnabled() const {
	return wordWrappingEnabled_;
}


void ou::Text::SetLineSpacing(float lineSpacing) {
	lineSpacing_ = lineSpacing;
	isLayoutDirty_ = true;
}


float ou::Text::GetLineSpacing() const {
	return lineSpacing_;
}


void ou::Text::SetCharacterSpacing(float characterSpacing) {
	characterSpacing_ = characterSpacing;
	isLayoutDirty_ = true;
}


float ou::Text::GetCharacterSpacing() const {
	return characterSpacing_;
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
		if (wordWrappingEnabled_ && !line.empty() && penX + advance > rect.size.x) {
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
	const auto rectTop = rect.position.y + rect.size.y;

	// 垂直アライメント
	auto blockTop = rectTop;
	if (vertical == 1) {
		blockTop = rectTop - (rect.size.y - blockHeight) * 0.5f;
	} else if (vertical == 2) {
		blockTop = rect.position.y + blockHeight;
	}
	const auto firstBaseline = blockTop - ascent;

	const auto atlasSize = static_cast<float>(font.GetAtlasSize());
	for (size_t i = 0; i < lines.size(); i++) {
		// 行のベースラインと横幅
		const auto &line = lines[i];
		const auto baseline = firstBaseline - lineHeight * static_cast<float>(i);
		const auto lineWidth = MeasureLineWidth(line);

		// 平行アライメント
		auto lineLeft = rect.position.x;
		if (horizontal == 1) {
			lineLeft += (rect.size.x - lineWidth) * 0.5f;
		} else if (horizontal == 2) {
			lineLeft += rect.size.x - lineWidth;
		}

		for (const auto &lineGlyph : line) {
			const auto &glyphRect = lineGlyph.glyph->glyphRect;
			if (glyphRect.size.x <= 0.0f) { continue; }

			// 描画する矩形の左上座標
			const auto &offset = lineGlyph.glyph->bitmapOffset;
			const auto left = lineLeft + lineGlyph.x + offset.x * scale;
			const auto top = baseline + offset.y * scale;

			// 描画する矩形を保存
			PlacedGlyph placed{};
			placed.positionRect = {
				left,
				top,
				left + glyphRect.size.x * scale,
				top - glyphRect.size.y * scale};
			placed.texcoordRect = {
				glyphRect.position.x / atlasSize,
				glyphRect.position.y / atlasSize,
				(glyphRect.position.x + glyphRect.size.x) / atlasSize,
				(glyphRect.position.y + glyphRect.size.y) / atlasSize};
			placed.atlasIndex = lineGlyph.glyph->atlasIndex;
			placedGlyphs_.push_back(placed);
		}
	}
}


void ou::Text::EnsureCapacity(uint32_t count) {
	// 容量が足りているか
	if (count <= capacity_) { return; }

	// 新しい容量を決める
	capacity_ = std::max({count, capacity_ * 2, 64u});

	// バッファの設定
	Graphics::GpuBufferDesc desc{};
	desc.size = capacity_ * sizeof(GlyphQuad);
	desc.heapType = Graphics::HeapType::UPLOAD;
	desc.bindFlags = Graphics::BindFlag::SHADER_RESOURCE;
	desc.miscFlags = Graphics::ResourceMiscFlag::BUFFER_STRUCTURED;
	desc.stride = sizeof(GlyphQuad);

	// バッファを作り直す
	for (auto &buffer : glyphBuffers_) {
		if (!Graphics::GetDevice()->CreateBuffer(&desc, nullptr, &buffer)) {
			FatalError("テキストのバッファの作成に失敗しました");
		}
	}
}
