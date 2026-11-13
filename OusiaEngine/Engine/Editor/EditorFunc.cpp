#include "EditorFunc.h"
#include "Graphics/DirectX12/GraphicsDevice_DX12.h"

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
