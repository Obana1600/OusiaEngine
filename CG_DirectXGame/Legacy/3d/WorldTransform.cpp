#include "WorldTransform.h"

#include "base/CreateBufferResource.h"
#include "base/DirectXCommon.h"

using namespace ou;


void WorldTransform::Initialize() {
	size_t size = (sizeof(ConstBufferData) + 255) & ~255;
	constBuffer_ = CreateBufferResource(DirectXCommon::GetInstance()->GetDevice(), size);

	constBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&constMap_));

	Update();
}


void WorldTransform::Update() {
	matWorld_ = Matrix4x4::SRT(scale_, rotation_, translation_);

	if (parent_) {
		matWorld_ *= parent_->matWorld_;
	}

	TransferMatrix();
}


void WorldTransform::TransferMatrix() {
	constMap_->matWorld = matWorld_;
}