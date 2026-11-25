#pragma once

#include <optional>

#include <imgui/imgui.h>


namespace ou {

// 押してから離すまでに、ImGui がドラッグとみなす距離を超えて動かしたか
inline bool WasMouseDragged() {
	const auto &io = ImGui::GetIO();
	return io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] >= io.MouseDragThreshold * io.MouseDragThreshold;
}

// Unity と同じく、項目を選ぶのはマウスを離したとき
// 押した瞬間に選ぶと Inspector が切り替わり、ドラッグで参照の欄へ引けない
class SelectOnRelease final {
public: // メンバ関数

	// 項目を描いた直後に呼ぶ。この項目を選ぶときに true を返す
	// 矢印で開いたときと、ドラッグしたときは選ばない
	bool TrackItem() {
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			isPressedOnArrow_ = ImGui::IsItemToggledOpen();
		}
		return ImGui::IsItemDeactivated() && !isPressedOnArrow_ && !WasMouseDragged();
	}


private: // メンバ変数

	bool isPressedOnArrow_ = false;
};

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
			if (!WasMouseDragged() && ImGui::IsItemHovered()) {
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
