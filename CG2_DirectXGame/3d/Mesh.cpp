#include "Mesh.h"

#include "Material.h"
#include "base/CreateBufferResource.h"
#include "base/DirectXCommon.h"
#include <cassert>


void Mesh::AddVertex(const VertexInfo &vertex) {
	vertices_.push_back(vertex);
}


void Mesh::AddIndex(uint32_t index) {
	indices_.push_back(index);
}


size_t Mesh::GetVertexCount() const {
	return vertices_.size();
}


void Mesh::SetMaterial(Material *material) {
	assert(material);
	material_ = material;
}


void Mesh::CreateBuffers() {
	auto *device = DirectXCommon::GetInstance()->GetDevice();

	// 頂点バッファ ---------------------------------------------------------------------------------

	auto vbSize = sizeof(VertexInfo) * vertices_.size();
	vertexResource_ = CreateBufferResource(device, vbSize);

	void *vbMap = nullptr;
	HRESULT hr = vertexResource_->Map(0, nullptr, &vbMap);
	std::memcpy(vbMap, vertices_.data(), vbSize);
	vertexResource_->Unmap(0, nullptr);

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = UINT(vbSize);
	vertexBufferView_.StrideInBytes = sizeof(VertexInfo);

	// --------------------------------------------------------------------------------------------


	// インデックスバッファ ---------------------------------------------------------------------------

	auto ibSize = sizeof(uint32_t) * indices_.size();
	indexResource_ = CreateBufferResource(device, ibSize);

	void *ibMap = nullptr;
	hr = indexResource_->Map(0, nullptr, &ibMap);
	std::memcpy(ibMap, indices_.data(), ibSize);
	indexResource_->Unmap(0, nullptr);

	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = UINT(ibSize);
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	// --------------------------------------------------------------------------------------------
}


void Mesh::Draw(
	ID3D12GraphicsCommandList *commandList, UINT rootParameterIndexMaterial, UINT rootParameterIndexTexture
) {
	material_->Update();
	material_->SetGraphicsCommand(commandList, rootParameterIndexMaterial, rootParameterIndexTexture);

	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->IASetIndexBuffer(&indexBufferView_);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(UINT(indices_.size()), 1, 0, 0, 0);
}


void Mesh::Draw(
	ID3D12GraphicsCommandList *commandList, UINT rootParameterIndexMaterial, UINT rootParameterIndexTexture, uint32_t textureHandle
) {
	material_->Update();
	material_->SetGraphicsCommand(commandList, rootParameterIndexMaterial, rootParameterIndexTexture, textureHandle);

	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->IASetIndexBuffer(&indexBufferView_);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(UINT(indices_.size()), 1, 0, 0, 0);
}