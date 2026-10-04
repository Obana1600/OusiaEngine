#include "DirectionLight.h"

#include "base/CreateBufferResource.h"
#include "base/DirectXCommon.h"

using namespace ou;


void DirectionLight::Initialize() {
	size_t size = (sizeof(ConstBufferData) + 255) & ~255;
	constBuffer_ = CreateBufferResource(DirectXCommon::GetInstance()->GetDevice(), size);
	constBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&constMap_));

	TransferData();
}


void DirectionLight::TransferData() {
	constMap_->color = color_;
	constMap_->direction = direction_;
	constMap_->intensity = intensity_;
}