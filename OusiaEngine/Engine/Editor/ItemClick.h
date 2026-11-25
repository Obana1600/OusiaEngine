#pragma once

#include <optional>

#include <imgui/imgui.h>


namespace ou {

// Unity・エクスプローラーの「選んでいる項目を、間を空けてもう一度クリックすると名前の変更」（スロークリック）
// ダブルクリックと区別するため、離してからダブルクリックの間隔（io.MouseDoubleClickTime）だけ待ち、
// その間にもう一度押されなければ、名前の変更を始める
template <typename Key>
class SlowClick final {
public: // メンバ関数

	// 項目を描いた直後に呼ぶ（IsItemClicked などで、直前の項目を見る）
	// wasSelected は、このクリックで選ぶ前から選ばれていたか
	void TrackItem(const Key &key, bool wasSelected) {
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			// ダブルクリックの 2 回目や、矢印で開いたときは数えない
			isArmed_ = wasSelected && !ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen();
			pressedKey_ = key;
			pendingKey_.reset();
		}

		if (isArmed_ && pressedKey_ == key && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			isArmed_ = false;

			// 押してから動かしていたら（ドラッグ）、名前の変更にしない
			const auto &io = ImGui::GetIO();
			const auto isDragged = io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] >= io.MouseDragThreshold * io.MouseDragThreshold;
			if (!isDragged && ImGui::IsItemHovered()) {
				pendingKey_ = key;
				pendingTime_ = ImGui::GetTime();
			}
		}
	}

	// 毎フレーム、項目を描いたあとに呼ぶ。名前の変更を始める項目を返す
	std::optional<Key> Update() {
		if (!pendingKey_) { return std::nullopt; }

		// ほかをクリックしたら（2 回目のクリックも含む）やめる
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			pendingKey_.reset();
			return std::nullopt;
		}

		if (ImGui::GetTime() - pendingTime_ < ImGui::GetIO().MouseDoubleClickTime) { return std::nullopt; }

		auto key = std::move(pendingKey_);
		pendingKey_.reset();
		return key;
	}

	void Cancel() {
		isArmed_ = false;
		pendingKey_.reset();
	}


private: // メンバ変数

	bool isArmed_ = false;
	Key pressedKey_{};

	std::optional<Key> pendingKey_;
	double pendingTime_ = 0.0;
};

} // namespace ou
