#include "GpuResource.h"

using namespace ou;


GpuResourceOld::GpuResourceOld(ID3D12Resource *resource, D3D12_RESOURCE_STATES currentState)
	: resource_(resource),
	  usageState_(currentState) {}


GpuResourceOld::~GpuResourceOld() {
	Destroy();
}


void GpuResourceOld::Destroy() {
	resource_.Reset();
	gpuVirtualAddress_ = 0;
	versionId_++;
}


ID3D12Resource *GpuResourceOld::Resource() const {
	return resource_.Get();
}


D3D12_GPU_VIRTUAL_ADDRESS GpuResourceOld::GpuVirtualAddress() const {
	return gpuVirtualAddress_;
}


uint32_t GpuResourceOld::VersionId() const {
	return versionId_;
}
