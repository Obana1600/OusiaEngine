#include "Image.h"
#include "Graphics/GraphicsDevice.h"
#include "Render/Asset/Texture.h"
#include "UI/RectTransform.h"
#include "UI/Sprite.h"

#include <utility>

using namespace ou;


void ou::Image::SetSprite(std::shared_ptr<Sprite> sprite) {
	sprite_ = std::move(sprite);
}


const std::shared_ptr<Sprite> &ou::Image::GetSprite() const {
	return sprite_;
}


void ou::Image::SetNativeSize() {
	if (!sprite_) { return; }
	GetRectTransform()->SetSizeDelta(sprite_->GetRect().size);
}


void ou::Image::Draw(Graphics::CommandList cmd, const Matrix4x4 &viewProj) const {
	const auto *rectTransform = GetRectTransform();
	const auto &rect = rectTransform->GetRect();

	Rect uvRect = {{}, {1.0f, 1.0f}};
	const auto *texture = GetWhiteTexture().get();
	if (sprite_) {
		uvRect = sprite_->MakeUvRect();
		texture = sprite_->GetTexture().get();
	}

	PerGraphic constants{};
	constants.worldViewProj = rectTransform->GetLocalToWorldMatrix() * viewProj;
	constants.uvTransform = Matrix4x4::Identity;
	constants.color = GetColor().Linear();
	constants.positionRect = {
		rect.position.x,
		rect.position.y + rect.size.y,
		rect.position.x + rect.size.x,
		rect.position.y};
	constants.texcoordRect = {
		uvRect.position.x,
		uvRect.position.y,
		uvRect.position.x + uvRect.size.x,
		uvRect.position.y + uvRect.size.y};
	constants.textureIndex = texture->GetDescriptorIndex();

	auto *device = Graphics::GetDevice();
	device->BindDynamicConstantBuffer(constants, 0, cmd);
	DrawWithMaterial(cmd, 4);
}
