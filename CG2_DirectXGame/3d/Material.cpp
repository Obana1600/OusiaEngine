#include "Material.h"

#include "base/CreateBufferResource.h"
#include "base/DirectXCommon.h"
#include "base/TextureManager.h"
#include "math/MathUtility.h"

using namespace MathUtility;


std::unique_ptr<Material> Material::Create() {
	std::unique_ptr<Material> material(new Material());
	material->Initialize();
	return material;
}


ID3D12Resource *Material::GetConstBuffer() const {
	return constBuffer_.Get();
}


void Material::LoadTexture(const std::string &directoryPath) {
	if (textureFileName_.empty()) {
		textureHandle_ = TextureManager::Load(kFallbackTextureName_);
		return;
	}

	textureHandle_ = TextureManager::Load(directoryPath + textureFileName_);
}


void Material::Update() {
	constMap_->uvTransform = MakeAffineMatrix(
		{uvScale_.x, uvScale_.y, 1.0f},
		{0.0f, 0.0f, uvRotate_},
		{uvOffset_.x, uvOffset_.y, 0.0f}
	);

	constMap_->color = color_;
	constMap_->alpha = alpha_;
	constMap_->lightMode = lightMode_;
}


void Material::SetGraphicsCommand(
	ID3D12GraphicsCommandList *commandList, UINT rootParameterIndexMaterial, UINT rootParameterIndexTexture
) {
	commandList->SetGraphicsRootConstantBufferView(
		rootParameterIndexMaterial,
		constBuffer_->GetGPUVirtualAddress()
	);

	TextureManager::GetInstance()->SetGraphicsRootDescriptorTable(
		commandList,
		rootParameterIndexTexture,
		textureHandle_
	);
}


void Material::SetGraphicsCommand(
	ID3D12GraphicsCommandList *commandList, UINT rootParameterIndexMaterial, UINT rootParameterIndexTexture, uint32_t textureHandle
) {
	textureHandle_ = textureHandle;

	commandList->SetGraphicsRootConstantBufferView(
		rootParameterIndexMaterial,
		constBuffer_->GetGPUVirtualAddress()
	);

	TextureManager::GetInstance()->SetGraphicsRootDescriptorTable(
		commandList,
		rootParameterIndexTexture,
		textureHandle_
	);
}


void Material::Initialize() {
	CreateConstBuffer();
}


void Material::CreateConstBuffer() {
	size_t size = (sizeof(ConstBufferData) + 255) & ~255;
	constBuffer_ = CreateBufferResource(DirectXCommon::GetInstance()->GetDevice(), size);
	constBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&constMap_));

	Update();
}