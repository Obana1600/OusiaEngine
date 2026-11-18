#include "EditorFunc.h"
#include "Core/PlayerSettings.h"
#include "Graphics/DirectX12/GraphicsDevice_DX12.h"

#include <algorithm>
#include <cmath>

#include <imgui/imgui.h>

using namespace ou;


uint64_t ou::ToTextureId(const Graphics::Texture &texture) {
#ifdef USE_IMGUI
	auto *device = static_cast<Graphics::GraphicsDevice_DX12 *>(Graphics::GetDevice());
	const auto index = device->GetDescriptorIndex(&texture, Graphics::ViewType::SRV);
	return device->GetBindlessGpuHandle(index).ptr;
#else
	(void)texture;
	return 0;
#endif
}


Vector2 ou::FitToPlayerAspect(const Vector2 &available) {
	const auto width = static_cast<float>(std::max(PlayerSettings::GetDefaultScreenWidth(), 1u));
	const auto height = static_cast<float>(std::max(PlayerSettings::GetDefaultScreenHeight(), 1u));
	const auto aspect = width / height;

	// 横いっぱいで高さが足りなければ、縦いっぱいにする
	Vector2 size{available.x, available.x / aspect};
	if (size.y > available.y) { size = {available.y * aspect, available.y}; }

	// 描き先は整数の大きさなので、切り捨ててから 1 以上にする
	return {std::max(std::floor(size.x), 1.0f), std::max(std::floor(size.y), 1.0f)};
}


void ou::CenterNextItem(const Vector2 &available, const Vector2 &size) {
	const auto cursor = ImGui::GetCursorPos();
	ImGui::SetCursorPos({cursor.x + std::floor((available.x - size.x) * 0.5f), cursor.y + std::floor((available.y - size.y) * 0.5f)});
}
