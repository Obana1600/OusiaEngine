#include "Sprite.h"
#include "Diagnostics/Log.h"
#include "Render/Asset/Texture.h"

#include <utility>

using namespace ou;


std::shared_ptr<Sprite> ou::Sprite::Create(std::shared_ptr<Texture> texture) {
	if (!texture) {
		Log::Error("テクスチャのないスプライトは作成できません");
		return nullptr;
	}

	const Rect rect = {
		{},
		{static_cast<float>(texture->GetWidth()), static_cast<float>(texture->GetHeight())}};

	return Create(std::move(texture), rect);
}


std::shared_ptr<Sprite> ou::Sprite::Create(std::shared_ptr<Texture> texture, const Rect &rect, const Vector2 &pivot) {
	if (!texture) {
		Log::Error("テクスチャのないスプライトは作成できません");
		return nullptr;
	}

	auto sprite = std::shared_ptr<Sprite>(new Sprite(texture, rect, pivot));
	sprite->SetName(texture->GetName());
	return sprite;
}


const std::shared_ptr<Texture> &ou::Sprite::GetTexture() const {
	return texture_;
}

const Rect &ou::Sprite::GetRect() const {
	return rect_;
}


const Vector2 &ou::Sprite::GetPivot() const {
	return pivot_;
}


Rect ou::Sprite::MakeUvRect() const {
	const Vector2 texSize = {static_cast<float>(texture_->GetWidth()),
							 static_cast<float>(texture_->GetHeight())};

	return {
		{rect_.position.x / texSize.x, rect_.position.y / texSize.y},
		{rect_.size.x / texSize.x, rect_.size.y / texSize.y}};
}


ou::Sprite::Sprite(std::shared_ptr<Texture> texture, const Rect &rect, const Vector2 &pivot)
	: texture_(std::move(texture)),
	  rect_(rect),
	  pivot_(pivot) {}
