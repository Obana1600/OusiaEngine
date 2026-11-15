#include "EditorGUILayout.h"

#include <algorithm>
#include <cfloat>

#include <imgui/imgui.h>

using namespace ou;


namespace {

// Inspector の幅の 45% から 40 を引く (最低 120)
float CalcLabelWidth(float viewWidth) {
	return std::max(viewWidth * 0.45f - 40.0f, 120.0f);
}

} // namespace



void ou::EditorGUILayout::PrefixLabel(std::string_view label) {
	const auto &style = ImGui::GetStyle();
	const auto startX = ImGui::GetCursorStartPos().x;
	const auto rowX = ImGui::GetCursorPosX();
	const auto viewWidth = ImGui::GetContentRegionAvail().x + (rowX - startX);

	const auto valueX = startX + CalcLabelWidth(viewWidth);
	const auto labelWidth = std::max(valueX - rowX - style.ItemSpacing.x, 1.0f);

	const auto *begin = label.data();
	const auto *end = label.data() + label.size();
	const auto pos = ImGui::GetCursorScreenPos();
	const auto height = ImGui::GetFrameHeight();
	auto *drawList = ImGui::GetWindowDrawList();
	drawList->PushClipRect(pos, {pos.x + labelWidth, pos.y + height}, true);
	drawList->AddText({pos.x, pos.y + style.FramePadding.y}, ImGui::GetColorU32(ImGuiCol_Text), begin, end);
	drawList->PopClipRect();

	ImGui::Dummy({labelWidth, height});
	if (ImGui::CalcTextSize(begin, end).x > labelWidth && ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%.*s", static_cast<int>(label.size()), begin);
	}

	ImGui::SameLine(valueX);
	ImGui::SetNextItemWidth(-FLT_MIN);
}
