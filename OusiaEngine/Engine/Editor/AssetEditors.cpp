#include "AssetEditors.h"
#include "Asset/AssetDatabase.h"
#include "Asset/Importer/TextureImporter.h"
#include "Audio/AudioClip.h"
#include "Core/EnumFlags.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorFunc.h"
#include "Editor/EditorGuiLayout.h"
#include "Editor/EditorGuiUtility.h"
#include "Editor/ExternalScriptEditor.h"
#include "PostProcess/VolumeProfile.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Shader.h"
#include "Render/Asset/Texture.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <optional>
#include <string>
#include <string_view>

#include <imgui/imgui.h>
#include <magic_enum/magic_enum.hpp>

using namespace ou;


namespace {

// 読み取りだけの欄（ラベルと値の文字）
void InfoField(std::string_view label, std::string_view value) {
	EditorGuiLayout::PrefixLabel(label);
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(value.data(), value.data() + value.size());
}


// ---- Texture ----

struct FormatInfo {
	const char *name;
	uint32_t bytesPerPixel;
};

FormatInfo GetFormatInfo(Graphics::Format format) {
	using enum Graphics::Format;
	switch (format) {
		default:
		case UNKNOWN:
			return {"Unknown", 0};

		case R32G32B32A32_FLOAT:
			return {"RGBA Float", 16};
		case R32G32B32_FLOAT:
			return {"RGB Float", 12};
		case R32G32_FLOAT:
			return {"RG Float", 8};
		case R32_FLOAT:
			return {"R Float", 4};
		case R16G16B16A16_FLOAT:
			return {"RGBA16 Float", 8};
		case R11G11B10_FLOAT:
			return {"RG11B10 Float", 4};

		case R32_UINT:
			return {"R32 UInt", 4};
		case R16_UINT:
			return {"R16 UInt", 2};

		case R8_UNORM:
			return {"R8", 1};

		case R8G8B8A8_UNORM:
			return {"RGBA8 UNorm", 4};
		case R8G8B8A8_UNORM_SRGB:
			return {"RGBA8 sRGB", 4};
		case B8G8R8A8_UNORM:
			return {"BGRA8 UNorm", 4};
		case B8G8R8A8_UNORM_SRGB:
			return {"BGRA8 sRGB", 4};

		case D32_FLOAT:
			return {"Depth 32", 4};
		case D24_UNORM_S8_UINT:
			return {"Depth 24 Stencil 8", 4};
	}
}

// GPU のメモリでの大きさ（ミップマップも含む）
uint64_t CalcTextureBytes(const Graphics::TextureDesc &desc) {
	const auto bytesPerPixel = GetFormatInfo(desc.format).bytesPerPixel;
	uint64_t bytes = 0;
	auto width = std::max(desc.width, 1u);
	auto height = std::max(desc.height, 1u);
	for (uint32_t mip = 0; mip < std::max(desc.mipLevels, 1u); mip++) {
		bytes += static_cast<uint64_t>(width) * height * bytesPerPixel;
		width = std::max(width / 2, 1u);
		height = std::max(height / 2, 1u);
	}
	return bytes * std::max(desc.arraySize, 1u) * std::max(desc.depth, 1u);
}

std::string FormatBytes(uint64_t bytes) {
	if (bytes >= 1024ull * 1024) { return std::format("{:.1f} MB", static_cast<double>(bytes) / (1024.0 * 1024.0)); }
	if (bytes >= 1024ull) { return std::format("{:.1f} KB", static_cast<double>(bytes) / 1024.0); }
	return std::format("{} B", bytes);
}

// ImGui の Image で描ける（普通の 2D の）テクスチャか。キューブマップや 3D はシェーダーで別の型として読むので描けない
bool CanDrawAsImage(const Graphics::TextureDesc &desc) {
	return desc.type == Graphics::TextureDesc::Type::TEXTURE_2D && desc.arraySize == 1 &&
		   !HasFlag(desc.miscFlags, Graphics::ResourceMiscFlag::TEXTURECUBE) &&
		   HasFlag(desc.bindFlags, Graphics::BindFlag::SHADER_RESOURCE);
}

// 透明なところが分かるように、市松模様を敷く（Unity の Preview と同じ）
void DrawCheckerboard(ImDrawList *drawList, const ImVec2 &min, const ImVec2 &max) {
	constexpr float kCell = 8.0f;
	const auto light = IM_COL32(120, 120, 120, 255);
	const auto dark = IM_COL32(80, 80, 80, 255);
	drawList->AddRectFilled(min, max, dark);

	auto row = 0;
	for (auto y = min.y; y < max.y; y += kCell, row++) {
		auto column = 0;
		for (auto x = min.x; x < max.x; x += kCell, column++) {
			if ((row + column) % 2 != 0) { continue; }
			drawList->AddRectFilled({x, y}, {std::min(x + kCell, max.x), std::min(y + kCell, max.y)}, light);
		}
	}
}


// ---- AudioClip ----

// 0:12.345
std::string FormatTime(double seconds) {
	const auto minutes = static_cast<int>(seconds / 60.0);
	return std::format("{}:{:06.3f}", minutes, seconds - minutes * 60.0);
}

const char *ChannelName(uint32_t channels) {
	switch (channels) {
		case 1:
			return "モノラル";
		case 2:
			return "ステレオ";
		default:
			return "マルチチャンネル";
	}
}


// ---- Serialize を持つアセット ----

// path の欄を描く。ラベルは日本語、ツールチップはファイルに書く名前
void Field(SerializedObject &so, std::string_view path, std::string_view label, std::string_view tooltip = {}) {
	auto property = so.FindProperty(path);
	if (!property) { return; }

	const auto hint = tooltip.empty() ? property->GetName() : std::format("{}（{}）", property->GetName(), tooltip);
	EditorGuiLayout::PropertyField(*property, EditorGuiUtility::TrTextContent(label, hint), true);
}

// シェーダの値を上書きする欄。チェックを入れたときだけ値の欄を出す
void OverrideField(SerializedObject &so, std::string_view name, std::string_view label) {
	const auto path = "renderStateOverride/" + std::string(name);
	const auto isOverridden = so.FindProperty(path + "/override");
	if (!isOverridden) { return; }

	Field(so, path + "/override", label, "チェックを入れると、シェーダの値の代わりに使う");
	if (!isOverridden->GetBoolValue()) { return; }

	ImGui::Indent();
	Field(so, path + "/value", "値");
	ImGui::Unindent();
}

// enum の名前（名前のない値なら ?）
template <typename E>
std::string_view EnumText(E value) {
	const auto name = magic_enum::enum_name(value);
	return name.empty() ? std::string_view("?") : name;
}

std::string RenderQueueText(int renderQueue) {
	switch (renderQueue) {
		case Shader::RenderQueue::BackGround:
			return "Background (1000)";
		case Shader::RenderQueue::Geometry:
			return "Geometry (2000)";
		case Shader::RenderQueue::AlphaTest:
			return "AlphaTest (2450)";
		case Shader::RenderQueue::Transparent:
			return "Transparent (3000)";
		case Shader::RenderQueue::Overlay:
			return "Overlay (4000)";
		default:
			return std::to_string(renderQueue);
	}
}

const char *PropertyTypeText(Shader::PropertyType type) {
	switch (type) {
		case Shader::PropertyType::Color:
			return "Color";
		case Shader::PropertyType::Vector:
			return "Vector";
		case Shader::PropertyType::Float:
			return "Float";
	}
	return "";
}


#ifdef WITH_EDITOR
// Assets の中の画像ファイルそのもののインポーター（モデルの中のテクスチャや EngineAssets のものは書き換えない）
TextureImporter *FindEditableTextureImporter(const Texture &texture) {
	const auto reference = AssetDatabase::FindAssetReference(&texture);
	if (!reference || !reference->subAsset.empty()) { return nullptr; }

	const auto assetPath = AssetDatabase::GuidToAssetPath(reference->guid);
	if (!assetPath.starts_with("Assets/")) { return nullptr; }

	return dynamic_cast<TextureImporter *>(AssetDatabase::GetImporterAtPath(assetPath));
}
#endif

} // namespace



// ---- TextureEditor ----

void ou::TextureEditor::OnInspectorGui() {
	const auto *texture = dynamic_cast<Texture *>(GetTarget());
	if (!texture) { return; }

	// インポートの設定（.meta に書き、変えたらすぐインポートし直す。SaveAndReimport はエディタだけ）
#ifdef WITH_EDITOR
	if (auto *importer = FindEditableTextureImporter(*texture)) {
		static const GuiContent kTypes[] = {
			{"Default", "3D モデルや UI に使う絵"},
			{"Cursor", "Cursor::SetCursor に渡す。ミップを作らず、CPU 側にも画素を残す"},
		};
		const auto current = static_cast<int>(importer->GetTextureType());
		const auto selected = EditorGuiLayout::Popup(GuiContent("種類", "Unity の Texture Type"), current, kTypes);
		if (selected != current) {
			importer->SetTextureType(static_cast<TextureImporterType>(selected));
			importer->SaveAndReimport();
			// target は古いオブジェクトになったので、ここで終える（次のフレームで Inspector が作り直す）
			return;
		}
	}
#endif

	const auto &desc = texture->GetGraphicsTexture().GetDesc();
	InfoField("大きさ", std::format("{} x {}", desc.width, desc.height));
	InfoField("形式", GetFormatInfo(desc.format).name);
	InfoField("ミップマップ", std::to_string(desc.mipLevels));
	InfoField("メモリ", FormatBytes(CalcTextureBytes(desc)));
	InfoField("CPU 側の画素", texture->IsReadable() ? "あり" : "なし");
}


bool ou::TextureEditor::HasPreviewGui() const {
	return dynamic_cast<Texture *>(GetTarget()) != nullptr;
}


void ou::TextureEditor::OnPreviewGui(const Rect &rect) {
	const auto *texture = dynamic_cast<Texture *>(GetTarget());
	if (!texture) { return; }

	const auto &graphicsTexture = texture->GetGraphicsTexture();
	const auto &desc = graphicsTexture.GetDesc();
	auto *drawList = ImGui::GetWindowDrawList();

	// 下に「大きさ・形式・メモリ」を 1 行出す（Unity の Preview の下の行）
	const auto info = std::format("{}x{}  {}  {}", desc.width, desc.height, GetFormatInfo(desc.format).name, FormatBytes(CalcTextureBytes(desc)));
	const auto infoHeight = ImGui::GetTextLineHeightWithSpacing();
	const auto infoWidth = ImGui::CalcTextSize(info.c_str()).x;
	drawList->AddText({rect.x + std::max((rect.width - infoWidth) * 0.5f, 0.0f), rect.y + rect.height - infoHeight},
					  ImGui::GetColorU32(ImGuiCol_Text), info.c_str());

	if (!CanDrawAsImage(desc)) {
		constexpr const char *kMessage = "このテクスチャの種類は Preview に出せません";
		const auto size = ImGui::CalcTextSize(kMessage);
		drawList->AddText({rect.x + (rect.width - size.x) * 0.5f, rect.y + (rect.height - infoHeight - size.y) * 0.5f},
						  ImGui::GetColorU32(ImGuiCol_TextDisabled), kMessage);
		return;
	}

	// 比を保ったまま、欄に収まるいちばん大きな大きさにする（文字の行の分は空ける）
	constexpr float kPadding = 4.0f;
	const auto areaWidth = rect.width - kPadding * 2.0f;
	const auto areaHeight = rect.height - infoHeight - kPadding * 2.0f;
	if (areaWidth <= 1.0f || areaHeight <= 1.0f || desc.width == 0 || desc.height == 0) { return; }

	const auto scale = std::min(areaWidth / static_cast<float>(desc.width), areaHeight / static_cast<float>(desc.height));
	const auto width = std::floor(static_cast<float>(desc.width) * scale);
	const auto height = std::floor(static_cast<float>(desc.height) * scale);
	const ImVec2 min{std::floor(rect.x + kPadding + (areaWidth - width) * 0.5f), std::floor(rect.y + kPadding + (areaHeight - height) * 0.5f)};
	const ImVec2 max{min.x + width, min.y + height};

	DrawCheckerboard(drawList, min, max);
	drawList->AddImage(ToTextureId(graphicsTexture), min, max);
}



// ---- AudioClipEditor ----

ou::AudioClipEditor::~AudioClipEditor() {
	// 別のものを選んだら止める（Unity と同じ）
	Stop();
}


void ou::AudioClipEditor::OnInspectorGui() {
	const auto *clip = dynamic_cast<AudioClip *>(GetTarget());
	if (!clip) { return; }

	// TODO: インポートの設定（AudioImporter）を編集できるようにする
	InfoField("チャンネル", std::format("{}（{}）", clip->GetChannels(), ChannelName(clip->GetChannels())));
	InfoField("サンプリングレート", std::format("{} Hz", clip->GetFrequency()));
	InfoField("長さ", FormatTime(clip->GetLength()));
	InfoField("サンプル数", std::to_string(clip->GetSamples()));
}


bool ou::AudioClipEditor::HasPreviewGui() const {
	return dynamic_cast<AudioClip *>(GetTarget()) != nullptr;
}


void ou::AudioClipEditor::OnPreviewSettings() {
	const auto isPlaying = IsPlaying();
	if (ImGui::SmallButton(isPlaying ? "Stop" : "Play")) {
		isPlaying ? Stop() : Play();
	}

	ImGui::SameLine();
	if (ImGui::Checkbox("Loop", &loops_) && IsPlaying()) {
		// ループは鳴らしはじめに決まるので、鳴らしなおす
		Play();
	}
}


void ou::AudioClipEditor::OnPreviewGui(const Rect &rect) {
	const auto *clip = dynamic_cast<AudioClip *>(GetTarget());
	if (!clip || rect.width < 2.0f || rect.height < 2.0f) { return; }

	const auto channels = std::max(clip->GetChannels(), 1u);
	const auto count = static_cast<uint32_t>(rect.width);
	if (count != peakCount_ || peaks_.size() != channels) {
		peakCount_ = count;
		peaks_.clear();
		for (uint32_t channel = 0; channel < channels; channel++) {
			peaks_.push_back(Audio::GetWaveformPeaks(clip->GetSound(), channel, count));
		}
	}

	auto *drawList = ImGui::GetWindowDrawList();
	const auto waveColor = IM_COL32(255, 140, 0, 255); // Unity の波形と同じ橙
	const auto laneHeight = rect.height / static_cast<float>(channels);
	for (uint32_t channel = 0; channel < channels; channel++) {
		const auto top = rect.y + laneHeight * static_cast<float>(channel);
		const auto center = top + laneHeight * 0.5f;
		const auto halfHeight = laneHeight * 0.5f - 2.0f;

		// チャンネルの境目と、0 の線
		if (channel > 0) { drawList->AddLine({rect.x, top}, {rect.x + rect.width, top}, IM_COL32(0, 0, 0, 255)); }
		drawList->AddLine({rect.x, center}, {rect.x + rect.width, center}, IM_COL32(90, 60, 20, 255));

		const auto &peaks = peaks_[channel];
		for (size_t i = 0; i < peaks.size(); i++) {
			const auto x = rect.x + static_cast<float>(i) + 0.5f;
			const auto y0 = center - peaks[i].max * halfHeight;
			const auto y1 = std::max(center - peaks[i].min * halfHeight, y0 + 1.0f); // 無音でも 1 ピクセルは描く
			drawList->AddLine({x, y0}, {x, y1}, waveColor);
		}
	}

	// 再生位置の線と時間
	if (IsPlaying() || Audio::IsPaused(&instance_)) {
		const auto samples = std::max(clip->GetSamples(), uint64_t{1});
		const auto position = Audio::GetPlaybackPosition(&instance_);
		const auto x = rect.x + rect.width * static_cast<float>(static_cast<double>(position) / static_cast<double>(samples));
		drawList->AddLine({x, rect.y}, {x, rect.y + rect.height}, IM_COL32(255, 255, 255, 255));

		const auto frequency = std::max(clip->GetFrequency(), 1u);
		const auto time = FormatTime(static_cast<double>(position) / frequency) + " / " + FormatTime(clip->GetLength());
		drawList->AddText({rect.x + 4.0f, rect.y + 2.0f}, IM_COL32(255, 255, 255, 255), time.c_str());
	} else {
		const auto time = FormatTime(clip->GetLength());
		drawList->AddText({rect.x + 4.0f, rect.y + 2.0f}, ImGui::GetColorU32(ImGuiCol_TextDisabled), time.c_str());
	}
}


void ou::AudioClipEditor::Play() {
	const auto *clip = dynamic_cast<AudioClip *>(GetTarget());
	if (!clip) { return; }

	// ループするかは作るときに決まるので、毎回作りなおす
	Stop();
	instance_ = {};
	instance_.loop = loops_;
	if (!Audio::CreateSoundInstance(&clip->GetSound(), &instance_)) { return; }
	Audio::Play(&instance_);
}


void ou::AudioClipEditor::Stop() {
	if (instance_.IsValid()) { Audio::Stop(&instance_); }
}


bool ou::AudioClipEditor::IsPlaying() const {
	return Audio::IsPlaying(&instance_);
}



// ---- SerializedAssetEditor ----

ou::SerializedAssetEditor::~SerializedAssetEditor() {
	// ドラッグ中に別のものを選んだときも、変えた値は残す
	if (hasUnsavedChanges_) { Save(); }
}


void ou::SerializedAssetEditor::OnEnable() {
	const auto reference = AssetDatabase::FindAssetReference(GetTarget());
	if (!reference || !reference->subAsset.empty()) { return; }

	const auto assetPath = AssetDatabase::GuidToAssetPath(reference->guid);
	if (assetPath.starts_with("Assets/")) { assetPath_ = assetPath; }
}


bool ou::SerializedAssetEditor::IsEditable() const {
	return !assetPath_.empty();
}


void ou::SerializedAssetEditor::DrawReadOnlyNote() const {
	if (IsEditable()) { return; }
	ImGui::TextDisabled("エンジンのものやモデルの中のものは、ここでは書き換えられません");
	ImGui::Separator();
}


void ou::SerializedAssetEditor::SaveWhenIdle(bool isModified) {
	hasUnsavedChanges_ = hasUnsavedChanges_ || isModified;
	if (hasUnsavedChanges_ && !ImGui::IsAnyItemActive()) { Save(); }
}


void ou::SerializedAssetEditor::Save() {
	hasUnsavedChanges_ = false;
	if (!IsEditable() || !GetTarget()) { return; }

	AssetDatabase::CreateAsset(*GetTarget(), assetPath_);
}



// ---- PhysicsMaterialEditor ----

void ou::PhysicsMaterialEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	DrawReadOnlyNote();

	ImGui::BeginDisabled(!IsEditable());
	Field(so, "dynamicFriction", "動摩擦", "動いているときの摩擦。0 以上");
	Field(so, "staticFriction", "静止摩擦", "止まっているときの摩擦。0 以上");
	Field(so, "bounciness", "反発", "0 で跳ねない、1 で同じ速さで跳ね返る");
	Field(so, "frictionCombine", "摩擦の合成", "ぶつかった 2 つの摩擦をどうまとめるか");
	Field(so, "bounceCombine", "反発の合成", "ぶつかった 2 つの反発をどうまとめるか");
	ImGui::EndDisabled();

	SaveWhenIdle(so.ApplyModifiedProperties());
}



// ---- MaterialEditor ----

void ou::MaterialEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	DrawReadOnlyNote();

	ImGui::BeginDisabled(!IsEditable());
	Field(so, "shader", "シェーダ", "替えると、プロパティはそのシェーダの初期値に戻る");
	Field(so, "renderQueue", "描画順", "-1 ならシェーダの値を使う");
	if (const auto *material = dynamic_cast<Material *>(GetTarget()); material && material->GetShader()) {
		ImGui::TextDisabled("今の描画順: %s", RenderQueueText(material->GetRenderQueue()).c_str());
	}

	ImGui::SeparatorText("見た目");
	Field(so, "mainTexture", "テクスチャ", "None なら白");
	Field(so, "color", "色");
	Field(so, "lightingMode", "ライティング");

	ImGui::SeparatorText("UV");
	Field(so, "uvOffset", "ずらす");
	Field(so, "uvScale", "拡大");
	Field(so, "uvRotation", "回転", "度");

	ImGui::SeparatorText("描画の設定（シェーダの値を上書き）");
	OverrideField(so, "srcBlend", "描く色に掛けるもの");
	OverrideField(so, "dstBlend", "今の色に掛けるもの");
	OverrideField(so, "blendOp", "混ぜ方");
	OverrideField(so, "zWrite", "深度を書く");
	OverrideField(so, "cull", "裏面を消す");

	// シェーダの "properties" に書いたもの
	if (const auto properties = so.FindProperty("properties"); properties && properties->HasVisibleChildren()) {
		ImGui::SeparatorText("シェーダのプロパティ");
		auto child = *properties;
		const auto depth = properties->GetDepth();
		for (auto enterChildren = true; child.NextVisible(enterChildren) && child.GetDepth() > depth; enterChildren = false) {
			EditorGuiLayout::PropertyField(child, true);
		}
	}
	ImGui::EndDisabled();

	SaveWhenIdle(so.ApplyModifiedProperties());
}



// ---- ShaderEditor ----

void ou::ShaderEditor::OnInspectorGui() {
	const auto *shader = dynamic_cast<Shader *>(GetTarget());
	if (!shader) { return; }

	// Unity と同じく、ここでは編集しない。HLSL やパスの設定はファイルを書き換える
	if (const auto reference = AssetDatabase::FindAssetReference(shader)) {
		const auto assetPath = AssetDatabase::GuidToAssetPath(reference->guid);
		if (ImGui::Button("開く")) {
			if (const auto fullPath = AssetDatabase::AssetPathToFullPath(assetPath)) { ExternalScriptEditor::OpenFile(*fullPath); }
		}
		ImGui::SameLine();
		ImGui::TextDisabled("書き換えたら、エディタを開き直すと読み直します");
	}

	InfoField("描画順", RenderQueueText(shader->GetRenderQueue()));
	InfoField("パスの数", std::to_string(shader->GetPassCount()));

	const auto &passes = shader->GetPasses();
	for (size_t i = 0; i < passes.size(); i++) {
		const auto &pass = passes[i];
		const auto title = std::format("Pass {}: {}##Pass{}", i, pass.name.empty() ? "(名前なし)" : pass.name, i);
		if (!ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) { continue; }

		ImGui::Indent();
		std::string stages;
		const auto addStage = [&](const Graphics::Shader &stage, std::string_view name) {
			if (!stage.IsValid()) { return; }
			if (!stages.empty()) { stages += ", "; }
			stages += name;
		};
		addStage(pass.vertexShader, "Vertex");
		addStage(pass.hullShader, "Hull");
		addStage(pass.domainShader, "Domain");
		addStage(pass.geometryShader, "Geometry");
		addStage(pass.pixelShader, "Pixel");
		InfoField("ステージ", stages);

		const auto &blend = pass.blend.renderTarget[0];
		InfoField("ブレンド", blend.blendEnable
								  ? std::format("{} {} {}", EnumText(blend.srcBlend), EnumText(blend.blendOp), EnumText(blend.destBlend))
								  : std::string("Off"));
		InfoField("深度を書く", pass.depthStencil.depthWriteMask == Graphics::DepthWriteMask::ALL ? "On" : "Off");
		InfoField("深度の比べ方", EnumText(pass.depthStencil.comparisonFunc));
		InfoField("裏面を消す", EnumText(pass.rasterizer.cullMode));
		ImGui::Unindent();
	}

	const auto &properties = shader->GetProperties();
	if (properties.empty()) { return; }

	ImGui::SeparatorText("プロパティ");
	for (const auto &property : properties) {
		const auto &value = property.defaultValue;
		const auto defaultText = property.type == Shader::PropertyType::Float
									 ? std::format("{:g}", value.x)
									 : std::format("({:g}, {:g}, {:g}, {:g})", value.x, value.y, value.z, value.w);
		InfoField(property.name, std::format("{}  初期値 {}", PropertyTypeText(property.type), defaultText));
	}
}



// ---- VolumeProfileEditor ----

void ou::VolumeProfileEditor::OnInspectorGui() {
	auto *profile = dynamic_cast<VolumeProfile *>(GetTarget());
	if (!profile) { return; }

	auto &so = *GetSerializedObject();
	so.Update();
	DrawReadOnlyNote();

	// 足す・消すと components の並びが変わるので、欄を全部描いて ApplyModifiedProperties してから行う
	std::string removing;
	std::string adding;

	ImGui::BeginDisabled(!IsEditable());
	const auto &components = profile->GetComponents();
	for (size_t i = 0; i < components.size(); i++) {
		if (!DrawComponent(so, *components[i], i)) { removing = VolumeComponent::FindTypeName(*components[i]); }
	}
	if (components.empty()) {
		ImGui::TextDisabled("効果がありません。Add Override で足してください");
	}

	ImGui::Spacing();
	if (ImGui::Button("Add Override", {-FLT_MIN, 0.0f})) { ImGui::OpenPopup("AddOverride"); }
	if (ImGui::BeginPopup("AddOverride")) {
		auto hasChoice = false;
		for (const auto typeName : VolumeComponent::GetTypeNames()) {
			if (profile->Has(typeName)) { continue; } // 型ごとに 1 つまで
			hasChoice = true;
			if (ImGui::MenuItem(std::string(typeName).c_str())) { adding = typeName; }
		}
		if (!hasChoice) { ImGui::TextDisabled("全部の効果を足してあります"); }
		ImGui::EndPopup();
	}
	ImGui::EndDisabled();

	auto isModified = so.ApplyModifiedProperties();
	if (!removing.empty()) {
		profile->Remove(removing);
		isModified = true;
	}
	if (!adding.empty()) {
		// Unity と同じく、足したばかりの効果はどのチェックも外れている（何も上書きしない）
		profile->Add(adding);
		isModified = true;
	}
	SaveWhenIdle(isModified);
}


bool ou::VolumeProfileEditor::DrawComponent(SerializedObject &so, const VolumeComponent &component, size_t index) {
	const auto path = std::format("components/{}", index);
	const auto typeName = std::string(VolumeComponent::FindTypeName(component));
	ImGui::PushID(typeName.c_str());

	// 見出しの × で消す（Unity は見出しの ⋮ の Remove）
	auto isKept = true;
	if (ImGui::CollapsingHeader(typeName.c_str(), &isKept, ImGuiTreeNodeFlags_DefaultOpen)) {
		Field(so, path + "/active", "有効", "外すと、この効果は働かない");

		// 全部のチェックを入れる・外す（Unity の All / None）
		std::optional<bool> overridesTo;
		if (ImGui::SmallButton("All")) { overridesTo = true; }
		ImGui::SameLine();
		if (ImGui::SmallButton("None")) { overridesTo = false; }

		for (size_t i = 0; i < component.parameters_.size(); i++) {
			const auto parameterPath = path + "/" + std::string(component.parameterNames_[i]);
			const auto parameter = so.FindProperty(parameterPath);
			auto overrideState = so.FindProperty(parameterPath + "/overrideState");
			auto value = so.FindProperty(parameterPath + "/value");
			if (!parameter || !overrideState || !value) { continue; }
			ImGui::PushID(static_cast<int>(i));

			if (overridesTo) { overrideState->SetBoolValue(*overridesTo); }

			// チェックを入れたものだけが、Volume のブレンドで値を上書きする。外したものは値の欄を灰色にする
			auto isOverridden = overrideState->GetBoolValue();
			if (ImGui::Checkbox("##Override", &isOverridden)) { overrideState->SetBoolValue(isOverridden); }
			ImGui::SetItemTooltip("チェックを入れると、この値を使う（外すと、既定の値か優先度の低い Volume の値のまま）");
			ImGui::SameLine();

			ImGui::BeginDisabled(!isOverridden);
			const auto label = parameter->GetDisplayName();
			if (const auto *clamped = dynamic_cast<const ClampedFloatParameter *>(component.parameters_[i])) {
				// 範囲のある値はスライダーにする（Unity と同じ）
				EditorGuiLayout::PrefixLabel(label);
				auto current = value->GetFloatValue();
				if (ImGui::SliderFloat("##Value", &current, clamped->GetMin(), clamped->GetMax(), "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
					value->SetFloatValue(current);
				}
			} else {
				EditorGuiLayout::PropertyField(*value, label);
			}
			ImGui::EndDisabled();

			ImGui::PopID();
		}
	}

	ImGui::PopID();
	return isKept;
}
