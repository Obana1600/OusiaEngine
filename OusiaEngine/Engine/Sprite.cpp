#include "Sprite.h"
#include "Core/Fatal.h"
#include "Graphics/DXCompiler.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/TextureManager.h"

#include <utility>

using namespace ou;


namespace {

Matrix4x4 MakeScreenOrtho(float width, float height) {
	return {
		2.0f / width, 0.0f, 0.0f, 0.0f,
		0.0f, -2.0f / height, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f, 1.0f};
}

} // namespace



void ou::Sprite::StaticInitialize(const RenderPassInfo &renderPass, uint32_t windowWidth, uint32_t windowHeight) {
}


void ou::Sprite::StaticFinalize() {
}


void ou::Sprite::PreDraw(CommandList cmd) {
}


std::unique_ptr<Sprite> ou::Sprite::Create(uint32_t textureHandle, Vector2 position, Vector4 color, Vector2 anchorPoint, bool isFlipX, bool isFlipY) {
	return std::unique_ptr<Sprite>();
}


void ou::Sprite::Draw(CommandList cmd) const {
}


void ou::Sprite::SetTextureHandle(uint32_t textureHandle) {
}


uint32_t ou::Sprite::GetTextureHandle() const {
	return 0;
}

void ou::Sprite::SetTranslation(const Vector2 &translation) {
}

const Vector2 &ou::Sprite::GetTranslation() const {
	// TODO: return ステートメントをここに挿入します
}


void ou::Sprite::SetRotation(const Vector3 &rotation) {
}


const Vector3 &ou::Sprite::GetRotation() const {
	// TODO: return ステートメントをここに挿入します
}


void ou::Sprite::SetScale(const Vector2 &scale) {
}


const Vector2 &ou::Sprite::GetScale() const {
	// TODO: return ステートメントをここに挿入します
}


void ou::Sprite::SetUvTranslation(const Vector2 &uvTranslation) {
}


const Vector2 &ou::Sprite::GetUvTranslation() const {
	// TODO: return ステートメントをここに挿入します
}


void ou::Sprite::SetUvRotation(float uvRotation) {
}


float ou::Sprite::GetUvRotation() const {
	return 0.0f;
}


void ou::Sprite::SetUvScale(const Vector2 &uvScale) {
}


const Vector2 &ou::Sprite::GetUvScale() const {
	// TODO: return ステートメントをここに挿入します
}


void ou::Sprite::SetSize(const Vector2 &size) {
}


const Vector2 &ou::Sprite::GetSize() const {
	// TODO: return ステートメントをここに挿入します
}


void ou::Sprite::SetAnchorPoint(const Vector2 &anchorPoint) {
}


const Vector2 &ou::Sprite::GetAnchorPoint() const {
	// TODO: return ステートメントをここに挿入します
}


void ou::Sprite::SetColor(const Vector4 &color) {
}


const Vector4 &ou::Sprite::GetColor() const {
	// TODO: return ステートメントをここに挿入します
}


void ou::Sprite::SetIsFlipX(bool isFlipX) {
}


bool ou::Sprite::IsFlipX() const {
	return isFlipX_;
}


void ou::Sprite::SetIsFlipY(bool isFlipY) {
}


bool ou::Sprite::IsFlipY() const {
	return isFlipY_;
}


void ou::Sprite::SetTextureRect(const Vector2 &texBase, const Vector2 &texSize) {
}


Sprite::Sprite(
	uint32_t textureHandle,
	const Vector2 &position,
	const Vector4 &color,
	const Vector2 &anchorPoint,
	bool isFlipX,
	bool isFlipY) {
}