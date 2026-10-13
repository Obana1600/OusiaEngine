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



void ou::Sprite::StaticInitialize(const Graphics::RenderPassInfo &renderPass, uint32_t windowWidth, uint32_t windowHeight) {
	sMatProj_ = MakeScreenOrtho(static_cast<float>(windowWidth), static_cast<float>(windowHeight));

	sVs_ = DXCompiler::GetInstance()->CompileEngineShader(L"Sprite.VS.hlsl", Graphics::ShaderStage::VS);
	sPs_ = DXCompiler::GetInstance()->CompileEngineShader(L"Sprite.PS.hlsl", Graphics::ShaderStage::PS);

	Graphics::BlendState blend{};
	blend.renderTarget[0].blendEnable = true;
	blend.renderTarget[0].destBlendAlpha = Graphics::Blend::ZERO;

	Graphics::PipelineStateDesc desc{};
	desc.vs = &sVs_;
	desc.ps = &sPs_;
	desc.bs = &blend;
	desc.pt = Graphics::PrimitiveTopology::TRIANGLESTRIP;
	if (!Graphics::GetDevice()->CreatePipelineState(&desc, &sPso_, &renderPass)) {
		FatalError("Sprite の PSO の作成に失敗しました。");
	}
}


void ou::Sprite::StaticFinalize() {
	sPso_ = {};
	sPs_ = {};
	sVs_ = {};
}


void ou::Sprite::PreDraw(Graphics::CommandList cmd) {
	Graphics::GetDevice()->BindPipelineState(&sPso_, cmd);
}


std::unique_ptr<Sprite> ou::Sprite::Create(uint32_t textureHandle, Vector2 position, Vector4 color, Vector2 anchorPoint, bool isFlipX, bool isFlipY) {
	return std::unique_ptr<Sprite>(new Sprite(textureHandle, position, color, anchorPoint, isFlipX, isFlipY));
}


void ou::Sprite::Draw(Graphics::CommandList cmd) const {
	auto *texMgr = TextureManager::GetInstance();
	const auto &texDesc = texMgr->GetTexture(textureHandle_).GetDesc();
	const auto texWidth = static_cast<float>(texDesc.width);
	const auto texHeight = static_cast<float>(texDesc.height);

	// アンカーポイント
	const auto left = -size_.x * anchorPoint_.x;
	const auto right = size_.x * (1.0f - anchorPoint_.x);
	const auto top = -size_.y * anchorPoint_.y;
	const auto bottom = size_.y * (1.0f - anchorPoint_.y);

	// テクスチャ範囲の正規化
	auto texLeft = texBase_.x / texWidth;
	auto texRight = (texBase_.x + texSize_.x) / texWidth;
	auto texTop = texBase_.y / texHeight;
	auto texBottom = (texBase_.y + texSize_.y) / texHeight;

	// 反転
	if (isFlipX_) { std::swap(texLeft, texRight); }
	if (isFlipY_) { std::swap(texTop, texBottom); }

	const auto world = Matrix4x4::SRT(
		{scale_.x, scale_.y, 1.0f},
		rotation_,
		{translation_.x, translation_.y, 0.0f});

	const auto uvTransform = Matrix4x4::SRT(
		{uvScale_.x, uvScale_.y, 1.0f},
		{0.0f, 0.0f, uvRotation_},
		{uvTranslation_.x, uvTranslation_.y, 0.0f});

	SpriteConstants constants{};
	constants.worldViewProj = world * sMatProj_;
	constants.uvTransform = uvTransform;
	constants.color = color_;
	constants.positionRect = {left, top, right, bottom};
	constants.texcoordRect = {texLeft, texTop, texRight, texBottom};
	constants.textureIndex = texMgr->GetDescriptorIndex(textureHandle_);

	auto *device = Graphics::GetDevice();
	device->BindDynamicConstantBuffer(constants, 0, cmd);
	device->Draw(kNumVertex, 0, cmd);
}


void ou::Sprite::SetTextureHandle(uint32_t textureHandle) {
	textureHandle_ = textureHandle;
}


uint32_t ou::Sprite::GetTextureHandle() const {
	return textureHandle_;
}


void ou::Sprite::SetTranslation(const Vector2 &translation) {
	translation_ = translation;
}


const Vector2 &ou::Sprite::GetTranslation() const {
	return translation_;
}


void ou::Sprite::SetRotation(const Vector3 &rotation) {
	rotation_ = rotation;
}


const Vector3 &ou::Sprite::GetRotation() const {
	return rotation_;
}


void ou::Sprite::SetScale(const Vector2 &scale) {
	scale_ = scale;
}


const Vector2 &ou::Sprite::GetScale() const {
	return scale_;
}


void ou::Sprite::SetUvTranslation(const Vector2 &uvTranslation) {
	uvTranslation_ = uvTranslation;
}


const Vector2 &ou::Sprite::GetUvTranslation() const {
	return uvTranslation_;
}


void ou::Sprite::SetUvRotation(float uvRotation) {
	uvRotation_ = uvRotation;
}


float ou::Sprite::GetUvRotation() const {
	return uvRotation_;
}


void ou::Sprite::SetUvScale(const Vector2 &uvScale) {
	uvScale_ = uvScale;
}


const Vector2 &ou::Sprite::GetUvScale() const {
	return uvScale_;
}


void ou::Sprite::SetSize(const Vector2 &size) {
	size_ = size;
}


const Vector2 &ou::Sprite::GetSize() const {
	return size_;
}


void ou::Sprite::SetAnchorPoint(const Vector2 &anchorPoint) {
	anchorPoint_ = anchorPoint;
}


const Vector2 &ou::Sprite::GetAnchorPoint() const {
	return anchorPoint_;
}


void ou::Sprite::SetColor(const Vector4 &color) {
	color_ = color;
}


const Vector4 &ou::Sprite::GetColor() const {
	return color_;
}


void ou::Sprite::SetIsFlipX(bool isFlipX) {
	isFlipX_ = isFlipX;
}


bool ou::Sprite::IsFlipX() const {
	return isFlipX_;
}


void ou::Sprite::SetIsFlipY(bool isFlipY) {
	isFlipY_ = isFlipY;
}


bool ou::Sprite::IsFlipY() const {
	return isFlipY_;
}


void ou::Sprite::SetTextureRect(const Vector2 &texBase, const Vector2 &texSize) {
	texBase_ = texBase;
	texSize_ = texSize;
}


Sprite::Sprite(
	uint32_t textureHandle,
	const Vector2 &position,
	const Vector4 &color,
	const Vector2 &anchorPoint,
	bool isFlipX,
	bool isFlipY)
	: textureHandle_(textureHandle),
	  translation_(position),
	  color_(color),
	  anchorPoint_(anchorPoint),
	  isFlipX_(isFlipX),
	  isFlipY_(isFlipY) {
	const auto &desc = TextureManager::GetInstance()->GetTexture(textureHandle).GetDesc();
	texSize_ = {static_cast<float>(desc.width), static_cast<float>(desc.height)};
	size_ = texSize_;
}