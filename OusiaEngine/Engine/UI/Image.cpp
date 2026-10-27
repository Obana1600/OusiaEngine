#include "Image.h"
#include "Render/Asset/Texture.h"
#include "UI/Asset/Sprite.h"
#include "UI/RectTransform.h"
#include "UI/VertexHelper.h"

#include <utility>

using namespace ou;


void ou::Image::SetSprite(std::shared_ptr<Sprite> sprite) {
	if (sprite_ == sprite) { return; }
	sprite_ = std::move(sprite);
	SetAllDirty();
}


const std::shared_ptr<Sprite> &ou::Image::GetSprite() const {
	return sprite_;
}


void ou::Image::SetNativeSize() {
	if (!sprite_) { return; }
	GetRectTransform()->SetSizeDelta(sprite_->GetRect().size);
}


const std::shared_ptr<Texture> &ou::Image::GetMainTexture() const {
	return sprite_ ? sprite_->GetTexture() : GetWhiteTexture();
}


void ou::Image::OnPopulateMesh(VertexHelper &vh) {
	if (!sprite_) {
		Graphic::OnPopulateMesh(vh);
		return;
	}

	const auto &rect = GetRectTransform()->GetRect();
	const auto left = rect.center.x;
	const auto bottom = rect.center.y;
	const auto right = left + rect.size.x;
	const auto top = bottom + rect.size.y;

	const auto uvRect = sprite_->MakeUvRect();
	const auto u0 = uvRect.center.x;
	const auto v0 = uvRect.center.y;
	const auto u1 = u0 + uvRect.size.x;
	const auto v1 = v0 + uvRect.size.y;

	const auto &color = GetColor();
	vh.Clear();
	vh.AddVert({left, bottom, 0.0f}, color, {u0, v1, 0.0f, 0.0f});
	vh.AddVert({left, top, 0.0f}, color, {u0, v0, 0.0f, 0.0f});
	vh.AddVert({right, top, 0.0f}, color, {u1, v0, 0.0f, 0.0f});
	vh.AddVert({right, bottom, 0.0f}, color, {u1, v1, 0.0f, 0.0f});
	vh.AddTriangle(0, 1, 2);
	vh.AddTriangle(2, 3, 0);
}
