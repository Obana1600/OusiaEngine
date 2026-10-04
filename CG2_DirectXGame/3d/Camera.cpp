#include "Camera.h"

#include "base/CreateBufferResource.h"
#include "base/DirectXCommon.h"

using namespace ou;


void Camera::Initialize() {
	size_t size = (sizeof(ConstBufferData) + 255) & ~255;
	constBuffer_ = CreateBufferResource(DirectXCommon::GetInstance()->GetDevice(), size);

	constBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&constMap_));

	Update();
}


void Camera::Update() {
	auto cameraMatrix = Matrix4x4::SRT({1.0f, 1.0f, 1.0f}, rotation_, translation_);
	matView_ = cameraMatrix.Inverse();
	matProjection_ = Matrix4x4::Perspective(fovAngleY_, aspectRatio_, nearZ_, farZ_);

	TransferMatrix();
}


void Camera::TransferMatrix() {
	constMap_->view = matView_;
	constMap_->projection = matProjection_;
}