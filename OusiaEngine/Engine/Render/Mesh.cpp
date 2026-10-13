#include "Mesh.h"
#include "Core/Assert.h"
#include "Core/Fatal.h"
#include "Graphics/GraphicsDevice.h"

#include <utility>

using namespace ou;


std::shared_ptr<Mesh> ou::Mesh::CreateCube() {
	struct Face {
		Vector3 normal;
		Vector3 up;
	};
	const Face faces[] = {
		{Vector3::Back, Vector3::Up},
		{Vector3::Forward, Vector3::Up},
		{Vector3::Left, Vector3::Up},
		{Vector3::Right, Vector3::Up},
		{Vector3::Up, Vector3::Forward},
		{Vector3::Down, Vector3::Back}};

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	for (const auto &face : faces) {
		const auto right = Vector3::Cross(face.up, face.normal * -1.0f);
		const auto center = face.normal * 0.5f;
		const auto base = static_cast<uint32_t>(vertices.size());

		// 左上 -> 右上 -> 右下 -> 左下
		vertices.push_back({center - right * 0.5f + face.up * 0.5f, face.normal, {0.0f, 0.0f}});
		vertices.push_back({center + right * 0.5f + face.up * 0.5f, face.normal, {1.0f, 0.0f}});
		vertices.push_back({center + right * 0.5f - face.up * 0.5f, face.normal, {1.0f, 1.0f}});
		vertices.push_back({center - right * 0.5f - face.up * 0.5f, face.normal, {0.0f, 1.0f}});

		// 時計回り
		indices.insert(indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
	}

	auto mesh = std::make_shared<Mesh>();
	mesh->SetName("Cube");
	mesh->SetVertices(std::move(vertices));
	mesh->SetIndices(std::move(indices));
	mesh->Upload();

	return mesh;
}


Graphics::InputLayout ou::Mesh::MakeInputLayout() {
	Graphics::InputLayout layout{};
	layout.elements = {
		{"POSITION", 0, Graphics::Format::R32G32B32_FLOAT},
		{"NORMAL", 0, Graphics::Format::R32G32B32_FLOAT},
		{"TEXCOORD", 0, Graphics::Format::R32G32_FLOAT}};

	return layout;
}


void ou::Mesh::Upload() {
	ASSERT(!vertices_.empty() && !indices_.empty(), "頂点とインデックスの両方が空です。");

	if (subMeshes_.empty()) {
		subMeshes_.push_back({0, static_cast<uint32_t>(indices_.size())});
	}

	auto *device = Graphics::GetDevice();

	// 頂点バッファの作成
	Graphics::GpuBufferDesc vbDesc{};
	vbDesc.size = vertices_.size() * sizeof(Vertex);
	vbDesc.bindFlags = Graphics::BindFlag::VERTEX_BUFFER;
	if (!device->CreateBuffer(&vbDesc, vertices_.data(), &vertexBuffer_)) {
		FatalError("Mesh の頂点バッファの作成に失敗しました。");
	}
	device->SetName(&vertexBuffer_, GetName() + "_VB");

	// インデックスバッファの作成
	Graphics::GpuBufferDesc ibDesc{};
	ibDesc.size = indices_.size() * sizeof(uint32_t);
	ibDesc.bindFlags = Graphics::BindFlag::INDEX_BUFFER;
	if (!device->CreateBuffer(&ibDesc, indices_.data(), &indexBuffer_)) {
		FatalError("Mesh のインデックスバッファの作成に失敗しました。");
	}
	device->SetName(&indexBuffer_, GetName() + "_IB");
}


void ou::Mesh::SetVertices(std::vector<Vertex> vertices) {
	vertices_ = std::move(vertices);
}


const std::vector<Mesh::Vertex> &ou::Mesh::GetVertices() const {
	return vertices_;
}


void ou::Mesh::SetIndices(std::vector<uint32_t> indices) {
	indices_ = std::move(indices);
}


const std::vector<uint32_t> &ou::Mesh::GetIndices() const {
	return indices_;
}


void ou::Mesh::SetSubMeshes(std::vector<SubMesh> subMeshes) {
	subMeshes_ = std::move(subMeshes);
}


const std::vector<Mesh::SubMesh> &ou::Mesh::GetSubMeshes() const {
	return subMeshes_;
}


void ou::Mesh::BindBuffers(Graphics::CommandList cmd) {
	auto *device = Graphics::GetDevice();
	const Graphics::GpuBuffer *vbs[] = {&vertexBuffer_};
	const uint32_t strides[] = {sizeof(Vertex)};
	const uint64_t offsets[] = {0};
	device->BindVertexBuffers(vbs, 0, 1, strides, offsets, cmd);
	device->BindIndexBuffer(&indexBuffer_, Graphics::IndexBufferFormat::UINT32, 0, cmd);
}


void ou::Mesh::DrawSubMesh(uint32_t subMeshIndex, Graphics::CommandList cmd) const {
	const auto &subMesh = subMeshes_[subMeshIndex];
	Graphics::GetDevice()->DrawIndexed(subMesh.indexCount, subMesh.indexStart, 0, cmd);
}
