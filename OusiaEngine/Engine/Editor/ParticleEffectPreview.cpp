#include "ParticleEffectPreview.h"
#include "Core/Time.h"
#include "Editor/EditorApplication.h"
#include "Editor/Selection.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleSystemRenderer.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"

#include <algorithm>
#include <cfloat>
#include <format>
#include <string>

#include <imgui/imgui.h>

using namespace ou;


namespace {

// 止まっていたウィンドウが動き出したときなどに、一度に大きく進まないようにする
constexpr float kMaxDeltaTime = 0.1f;

constexpr float kPanelWidth = 260.0f;
constexpr float kPanelMargin = 8.0f;

bool HasParticleSystem(const GameObject *gameObject) {
	return gameObject && gameObject->GetComponent<ParticleSystem>() != nullptr;
}

// 動かす対象か（非アクティブなものは Play 中も動かないので、動かさない）
bool IsSimulated(const ParticleSystem *system) {
	return system && !system->IsDestroyed() && system->GetGameObject()->IsActiveInHierarchy();
}

} // namespace



void ou::ParticleEffectPreview::Update() {
	// Play 中は本物の Update が動かす。シーンも作り直されるので、持っているものは忘れる
	if (EditorApplication::IsPlaying()) {
		sRootId_ = 0;
		sSystemIds_.clear();
		sState_ = State::Stopped;
		sPlaybackTime_ = 0.0f;
		return;
	}

	// 選び直したら、前のものは止めて消し、新しいものを頭から再生する（Unity と同じく、選ぶと自動で再生する）
	auto *root = FindRoot(Selection::GetActiveGameObject());
	const auto rootId = root ? root->GetInstanceId() : 0u;
	if (rootId != sRootId_) {
		Stop();
		sRootId_ = rootId;
		sSystemIds_.clear();
		if (root) { CollectSystems(root); }
		if (!sSystemIds_.empty()) { Play(); }
		return;
	}
	if (!root) { return; }

	// 子を足したり消したりしたときのために、毎フレーム集め直す（軽い）
	sSystemIds_.clear();
	CollectSystems(root);

	if (sState_ == State::Playing) {
		const auto deltaTime = std::min(Time::GetUnscaledDeltaTime(), kMaxDeltaTime) * sPlaybackSpeed_;
		for (auto *system : GetSystems()) {
			system->Simulate(deltaTime);
		}
		sPlaybackTime_ += deltaTime;
	}
	RefreshRenderers();
}


void ou::ParticleEffectPreview::DrawPanel(const Rect &sceneRect) {
	if (sRootId_ == 0 || sSystemIds_.empty() || EditorApplication::IsPlaying()) { return; }

	const auto systems = GetSystems();
	if (systems.empty()) { return; }

	// 行の数から高さを決めて、右下に置く
	const auto &style = ImGui::GetStyle();
	constexpr int kRows = 5;
	const auto height = ImGui::GetFrameHeightWithSpacing() * kRows + style.WindowPadding.y * 2.0f;
	const ImVec2 pos{sceneRect.x + sceneRect.width - kPanelWidth - kPanelMargin, sceneRect.y + sceneRect.height - height - kPanelMargin};
	if (pos.x < sceneRect.x || pos.y < sceneRect.y) { return; } // 狭すぎるときは出さない

	// カーソルは戻さない（戻すと、項目を置かずに位置だけ動かしたことになり ImGui に怒られる）。Scene ビューはこの後に項目を置かない
	ImGui::SetCursorScreenPos(pos);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(40, 40, 40, 230));
	if (ImGui::BeginChild("##ParticleEffect", {kPanelWidth, height}, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
		const auto *root = systems.front()->GetGameObject();
		ImGui::Text("Particle Effect");
		ImGui::SameLine();
		ImGui::TextDisabled("%s", root->GetName().c_str());

		// Unity と同じく Play と Pause は 1 つのボタンを切り替える
		const auto buttonWidth = (ImGui::GetContentRegionAvail().x - style.ItemSpacing.x * 2.0f) / 3.0f;
		if (ImGui::Button(sState_ == State::Playing ? "Pause" : "Play", {buttonWidth, 0.0f})) {
			sState_ == State::Playing ? Pause() : Play();
		}
		ImGui::SameLine();
		if (ImGui::Button("Restart", {buttonWidth, 0.0f})) { Restart(); }
		ImGui::SameLine();
		if (ImGui::Button("Stop", {buttonWidth, 0.0f})) { Stop(); }

		constexpr float kLabelWidth = 110.0f;
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Playback Speed");
		ImGui::SetItemTooltip("再生の速さ（1 で等倍）");
		ImGui::SameLine(kLabelWidth);
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::DragFloat("##Speed", &sPlaybackSpeed_, 0.01f, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Playback Time");
		ImGui::SetItemTooltip("再生を始めてからの時間（秒）");
		ImGui::SameLine(kLabelWidth);
		ImGui::Text("%.2f", sPlaybackTime_);

		size_t count = 0;
		for (const auto *system : systems) { count += system->GetParticles().size(); }
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Particles");
		ImGui::SetItemTooltip("今出ている粒の数（子のパーティクルも含む）");
		ImGui::SameLine(kLabelWidth);
		ImGui::Text("%zu", count);
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();
}


GameObject *ou::ParticleEffectPreview::FindRoot(GameObject *selected) {
	if (!HasParticleSystem(selected)) { return nullptr; }

	// 親も ParticleSystem なら、1 つのエフェクトとしてまとめて動かす（Unity と同じ）
	auto *root = selected;
	for (auto *parent = root->GetTransform()->GetParent(); parent; parent = parent->GetParent()) {
		if (!HasParticleSystem(parent->GetGameObject())) { break; }
		root = parent->GetGameObject();
	}
	return root;
}


void ou::ParticleEffectPreview::CollectSystems(GameObject *gameObject) {
	if (auto *system = gameObject->GetComponent<ParticleSystem>()) {
		sSystemIds_.push_back(system->GetInstanceId());
	}
	for (auto *child : gameObject->GetTransform()->GetChildren()) {
		CollectSystems(child->GetGameObject());
	}
}


std::vector<ParticleSystem *> ou::ParticleEffectPreview::GetSystems() {
	std::vector<ParticleSystem *> systems;
	for (const auto id : sSystemIds_) {
		auto *system = dynamic_cast<ParticleSystem *>(Object::FindObjectFromInstanceId(id));
		if (IsSimulated(system)) { systems.push_back(system); }
	}
	return systems;
}


void ou::ParticleEffectPreview::Play() {
	if (sState_ == State::Paused) {
		sState_ = State::Playing;
		return;
	}

	for (auto *system : GetSystems()) {
		system->Clear();
		system->Play();
	}
	sPlaybackTime_ = 0.0f;
	sState_ = State::Playing;
}


void ou::ParticleEffectPreview::Pause() {
	if (sState_ == State::Playing) { sState_ = State::Paused; }
}


void ou::ParticleEffectPreview::Restart() {
	sState_ = State::Stopped;
	Play();
}


void ou::ParticleEffectPreview::Stop() {
	for (auto *system : GetSystems()) {
		system->Stop();
		system->Clear();
	}
	RefreshRenderers();
	sPlaybackTime_ = 0.0f;
	sState_ = State::Stopped;
}


void ou::ParticleEffectPreview::RefreshRenderers() {
	for (auto *system : GetSystems()) {
		if (auto *renderer = system->GetGameObject()->GetComponent<ParticleSystemRenderer>()) {
			renderer->LateUpdate();
		}
	}
}
