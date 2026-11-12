#include "Image.h"
#include "Render/Asset/Texture.h"
#include "UI/Asset/Sprite.h"
#include "UI/RectTransform.h"
#include "UI/VertexHelper.h"
#include "Serialize/Serializer.h"

#include <utility>

using namespace ou;


void ou::Image::Serialize(Serializer &serializer) {
	Graphic::Serialize(serializer);

	// TODO: Sprite をサブアセットにする
	auto texture = sprite_ ? sprite_->GetTexture() : nullptr;
	auto rect = sprite_ ? sprite_->GetRect() : Rect::Zero;
	auto pivot = sprite_ ? sprite_->GetPivot() : Vector2{0.5f, 0.5f};

	const auto oldTexture = texture;
	const auto oldRect = rect;
	const auto oldPivot = pivot;

	serializer.Struct("sprite", [&] {
		serializer.Asset("texture", texture);
		serializer.Field("rect", rect);
		serializer.Field("pivot", pivot);
	});

	if (!serializer.IsReading()) { return; }

	if (texture == oldTexture && rect == oldRect && pivot == oldPivot) { return; }

	SetSprite(texture ? Sprite::Create(texture, rect, pivot) : nullptr);
}


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
	GetRectTransform()->SetSizeDelta(sprite_->GetRect().GetSize());
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
	const auto left = rect.GetXMin();
	const auto bottom = rect.GetYMin();
	const auto right = rect.GetXMax();
	const auto top = rect.GetYMax();

	const auto uvRect = sprite_->MakeUvRect();
	const auto u0 = uvRect.GetXMin();
	const auto v0 = uvRect.GetYMin();
	const auto u1 = uvRect.GetXMax();
	const auto v1 = uvRect.GetYMax();

	const auto &color = GetColor();
	vh.Clear();
	vh.AddVert({left, bottom, 0.0f}, color, {u0, v1, 0.0f, 0.0f});
	vh.AddVert({left, top, 0.0f}, color, {u0, v0, 0.0f, 0.0f});
	vh.AddVert({right, top, 0.0f}, color, {u1, v0, 0.0f, 0.0f});
	vh.AddVert({right, bottom, 0.0f}, color, {u1, v1, 0.0f, 0.0f});
	vh.AddTriangle(0, 1, 2);
	vh.AddTriangle(2, 3, 0);
}
