#include "AssetEditors.h"
#include "Audio/AudioClip.h"
#include "Core/EnumFlags.h"
#include "Editor/EditorFunc.h"
#include "Editor/EditorGuiLayout.h"
#include "Render/Asset/Texture.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <string_view>

#include <imgui/imgui.h>

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
	case R32G32B32A32_FLOAT: return {"RGBA Float", 16};
	case R32G32B32_FLOAT: return {"RGB Float", 12};
	case R32G32_FLOAT: return {"RG Float", 8};
	case R32_FLOAT: return {"R Float", 4};
	case R32_UINT: return {"R32 UInt", 4};
	case R16_UINT: return {"R16 UInt", 2};
	case R8_UNORM: return {"R8", 1};
	case R8G8B8A8_UNORM: return {"RGBA8 UNorm", 4};
	case R8G8B8A8_UNORM_SRGB: return {"RGBA8 sRGB", 4};
	case B8G8R8A8_UNORM: return {"BGRA8 UNorm", 4};
	case B8G8R8A8_UNORM_SRGB: return {"BGRA8 sRGB", 4};
	case D32_FLOAT: return {"Depth 32", 4};
	case D24_UNORM_S8_UINT: return {"Depth 24 Stencil 8", 4};
	default: return {"Unknown", 0};
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
	case 1: return "モノラル";
	case 2: return "ステレオ";
	default: return "マルチチャンネル";
	}
}

} // namespace



// ---- TextureEditor ----

void ou::TextureEditor::OnInspectorGui() {
	const auto *texture = dynamic_cast<Texture *>(GetTarget());
	if (!texture) { return; }

	// TODO: インポートの設定（TextureImporter）を編集できるようにする
	const auto &desc = texture->GetGraphicsTexture().GetDesc();
	InfoField("大きさ", std::format("{} x {}", desc.width, desc.height));
	InfoField("形式", GetFormatInfo(desc.format).name);
	InfoField("ミップマップ", std::to_string(desc.mipLevels));
	InfoField("メモリ", FormatBytes(CalcTextureBytes(desc)));
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
