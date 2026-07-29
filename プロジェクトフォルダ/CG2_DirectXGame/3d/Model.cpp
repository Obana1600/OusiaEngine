#include "Model.h"

#include "Camera.h"
#include "WorldTransform.h"
#include "base/DXCompiler.h"
#include "base/DirectXCommon.h"
#include <Windows.h>
#include <cassert>
#include <fstream>
#include <sstream>


ModelCommon *ModelCommon::GetInstance() {
	static ModelCommon instance;
	return &instance;
}


void ModelCommon::Initialize() {
	directionLight_.Initialize();

	CompileShaders();
	CreateRootSignature();
	CreatePipelineState();
}


void ModelCommon::Finalize() {
}


void ModelCommon::TransformCommand(const WorldTransform &worldTransform, const Camera &camera) {
	commandList_->SetGraphicsRootConstantBufferView(
		static_cast<UINT>(Model::RootParameter::WorldTransform),
		worldTransform.GetConstBuffer()->GetGPUVirtualAddress()
	);

	commandList_->SetGraphicsRootConstantBufferView(
		static_cast<UINT>(Model::RootParameter::Camera),
		camera.GetConstBuffer()->GetGPUVirtualAddress()
	);
}


void ModelCommon::PreDraw(ID3D12GraphicsCommandList *commandList) {
	commandList_ = commandList;

	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(pipelineState_.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	directionLight_.TransferData();
	commandList_->SetGraphicsRootConstantBufferView(
		static_cast<UINT>(Model::RootParameter::Light),
		directionLight_.GetConstBuffer()->GetGPUVirtualAddress()
	);
}


void ModelCommon::PostDraw() {
	commandList_ = nullptr;
}


void ModelCommon::CompileShaders() {
	auto dxCompiler = DXCompiler::GetInstance();

	vsBlob_ = dxCompiler->CompileShader(L"Model.VS.hlsl", L"vs_6_0");
	assert(vsBlob_);

	psBlob_ = dxCompiler->CompileShader(L"Model.PS.hlsl", L"ps_6_0");
	assert(psBlob_);
}


void ModelCommon::CreateRootSignature() {
	auto device = DirectXCommon::GetInstance()->GetDevice();

	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// テクスチャ用のsrvレンジ
	D3D12_DESCRIPTOR_RANGE range[1]{};
	range[0].BaseShaderRegister = 0;
	range[0].NumDescriptors = 1;
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER params[static_cast<int>(Model::RootParameter::Count)]{};
	// worldTransform: b0
	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	params[0].Descriptor.ShaderRegister = 0;

	// camera: b1
	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	params[1].Descriptor.ShaderRegister = 1;

	// material: b2
	params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	params[2].Descriptor.ShaderRegister = 2;

	// light: b3
	params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	params[3].Descriptor.ShaderRegister = 3;

	// texture: t0
	params[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	params[4].DescriptorTable.pDescriptorRanges = range;
	params[4].DescriptorTable.NumDescriptorRanges = _countof(range);

	desc.pParameters = params;
	desc.NumParameters = _countof(params);

	// sampler: s0
	D3D12_STATIC_SAMPLER_DESC sampler[1]{};
	sampler[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler[0].MaxLOD = D3D12_FLOAT32_MAX;
	sampler[0].ShaderRegister = 0;
	sampler[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	desc.pStaticSamplers = sampler;
	desc.NumStaticSamplers = _countof(sampler);

	// シリアライズ -> 生成
	Microsoft::WRL::ComPtr<ID3DBlob> sig;
	Microsoft::WRL::ComPtr<ID3DBlob> err;

	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err);
	if (FAILED(hr)) {
		if (err) {
			OutputDebugStringA(reinterpret_cast<char *>(err->GetBufferPointer()));
		}
		assert(false);
	}

	hr = device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}


void ModelCommon::CreatePipelineState() {
	auto device = DirectXCommon::GetInstance()->GetDevice();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};

	D3D12_INPUT_ELEMENT_DESC inputElements[3]{};
	// POSITION0 float3
	inputElements[0].SemanticName = "POSITION";
	inputElements[0].SemanticIndex = 0;
	inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	// NOMRAL0 float3
	inputElements[1].SemanticName = "NORMAL";
	inputElements[1].SemanticIndex = 0;
	inputElements[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	// TEXCOORD0 float2
	inputElements[2].SemanticName = "TEXCOORD";
	inputElements[2].SemanticIndex = 0;
	inputElements[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	// inputLayout
	D3D12_INPUT_LAYOUT_DESC inputLayout{};
	inputLayout.pInputElementDescs = inputElements;
	inputLayout.NumElements = _countof(inputElements);
	desc.InputLayout = inputLayout;

	// blendState: アルファブレンドのみ
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	desc.BlendState = blendDesc;

	// rasterizerState
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	desc.RasterizerState = rasterizerDesc;

	// depthStencilState
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	desc.DepthStencilState = depthStencilDesc;

	desc.pRootSignature = rootSignature_.Get();
	desc.VS = {vsBlob_->GetBufferPointer(), vsBlob_->GetBufferSize()};
	desc.PS = {psBlob_->GetBufferPointer(), psBlob_->GetBufferSize()};
	desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.SampleDesc.Count = 1;
	desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));
}


void Model::InitializeFromFile(const std::string &modelName) {
	name_ = modelName;
	LoadModel(modelName);
	LoadTexture();
}


void Model::LoadModel(const std::string &modelName) {
	std::string directoryPath = kBaseDirectory + modelName + "/";

	// ファイルを開く
	std::ifstream file(directoryPath + modelName + ".obj");
	assert(file.is_open());

	// 頂点属性
	std::vector<Vector3> positions;
	std::vector<Vector2> texcoords;
	std::vector<Vector3> normals;

	meshes_.push_back(std::make_unique<Mesh>());
	auto *mesh = meshes_.back().get();

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream s(line);
		std::string key;
		s >> key;

		if (key == "v") {
			Vector3 position{};
			s >> position.x >> position.y >> position.z;
			position.x *= -1.0f; // x軸反転
			positions.push_back(position);

		} else if (key == "vt") {
			Vector2 texcoord{};
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y; // uv上下反転
			texcoords.push_back(texcoord);

		} else if (key == "vn") {
			Vector3 normal{};
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1.0f; // x軸反転
			normals.push_back(normal);

		} else if (key == "o") {
			if (mesh->GetVertexCount() > 0) {
				meshes_.push_back(std::make_unique<Mesh>());
				mesh = meshes_.back().get();
			}
			std::string name;
			s >> name;
			mesh->SetName(name);

		} else if (key == "usemtl") {
			std::string mtlName;
			s >> mtlName;

			if (mesh->GetVertexCount() > 0 && mesh->GetMaterial() != nullptr) {
				meshes_.push_back(std::make_unique<Mesh>());
				mesh = meshes_.back().get();
			}

			auto it = materials_.find(mtlName);
			if (it != materials_.end()) {
				mesh->SetMaterial(it->second.get());
			}

		} else if (key == "mtllib") {
			std::string mtlFile;
			s >> mtlFile;
			LoadMaterial(directoryPath, mtlFile);

		} else if (key == "f") {
			uint32_t faceVertexCount = 0;
			std::string element;

			while (s >> element) {
				std::istringstream es(element);
				std::string idxStr;
				uint32_t idx[3] = {0, 0, 0};
				for (int i = 0; i < 3; i++) {
					if (!std::getline(es, idxStr, '/')) {
						break;
					}
					if (!idxStr.empty()) {
						idx[i] = uint32_t(std::stoi(idxStr));
					}
				}

				Mesh::VertexInfo v{};
				v.pos = positions[idx[0] - 1];
				if (idx[2] > 0) {
					v.normal = normals[idx[2] - 1];
				}
				if (idx[1] > 0) {
					v.uv = texcoords[idx[1] - 1];
				}
				mesh->AddVertex(v);

				uint32_t cur = uint32_t(mesh->GetVertexCount()) - 1;
				if (faceVertexCount >= 2) {
					uint32_t first = cur - faceVertexCount;
					mesh->AddIndex(first);
					mesh->AddIndex(cur);
					mesh->AddIndex(cur - 1);
				}
				faceVertexCount++;
			}
		}
	}

	for (auto &m : meshes_) {
		m->CreateBuffers();
	}
}


void Model::LoadMaterial(const std::string &directoryPath, const std::string &fileName) {
	std::ifstream file(directoryPath + fileName);
	assert(file.is_open());

	std::unique_ptr<Material> material = nullptr;

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream s(line);
		std::string key;
		s >> key;

		if (key == "newmtl") {
			if (material) {
				std::string name = material->name_;
				materials_.emplace(name, std::move(material));
			}
			material = Material::Create();
			s >> material->name_;

		} else if (key == "Kd") {
			s >> material->color_.x >> material->color_.y >> material->color_.z;

		} else if (key == "d") {
			s >> material->alpha_;

		} else if (key == "map_Kd") {
			s >> material->textureFileName_;
		}
	}

	if (material) {
		std::string name = material->name_;
		materials_.emplace(name, std::move(material));
	}
}


void Model::LoadTexture() {
	for (auto &it : materials_) {
		it.second->LoadTexture(name_ + "/");
	}
}


void Model::StaticInitialize() {
	ModelCommon::GetInstance()->Initialize();
}


void Model::StaticFinalize() {
	ModelCommon::GetInstance()->Finalize();
}


Model *Model::Create() {
	return new Model();
}


Model *Model::CreateFromObj(const std::string &modelName) {
	Model *model = new Model();
	model->InitializeFromFile(modelName);
	return model;
}


void Model::PreDraw() {
	auto commandList = DirectXCommon::GetInstance()->GetCommandListForRender();
	ModelCommon::GetInstance()->PreDraw(commandList);
}


void Model::PostDraw() {
	ModelCommon::GetInstance()->PostDraw();
}


void Model::Draw(const WorldTransform &worldTransform, const Camera &camera) {
	auto commandList = DirectXCommon::GetInstance()->GetCommandListForRender();

	ModelCommon::GetInstance()->TransformCommand(worldTransform, camera);

	for (auto &mesh : meshes_) {
		mesh->Draw(
			commandList,
			static_cast<UINT>(RootParameter::Material),
			static_cast<UINT>(RootParameter::Texture)
		);
	}
}


void Model::Draw(const WorldTransform &worldTransform, const Camera &camera, uint32_t textureHandle) {
	auto commandList = DirectXCommon::GetInstance()->GetCommandListForRender();

	ModelCommon::GetInstance()->TransformCommand(worldTransform, camera);

	for (auto &mesh : meshes_) {
		mesh->Draw(
			commandList,
			static_cast<UINT>(RootParameter::Material),
			static_cast<UINT>(RootParameter::Texture),
			textureHandle
		);
	}
}