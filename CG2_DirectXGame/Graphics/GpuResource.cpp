#include "GpuResource.h"

using namespace ou;


GpuResource::GpuResource(ID3D12Resource *resource, D3D12_RESOURCE_STATES currentState)
	: resource_(resource),
	  usageState_(currentState) {}


GpuResource::~GpuResource() {
	Destroy();
}


void GpuResource::Destroy() {
	resource_.Reset();
	gpuVirtualAddress_ = 0;
	versionId_++;
}


ID3D12Resource *GpuResource::Resource() const {
	return resource_.Get();
}


D3D12_GPU_VIRTUAL_ADDRESS GpuResource::GpuVirtualAddress() const {
	return gpuVirtualAddress_;
}


uint32_t GpuResource::VersionId() const {
	return versionId_;
}
