#include "GameScene.h"
#include "VertexData.h"
#include "base/DirectXCommon.h"
#include "base/TextureManager.h"
#include "TransformationMatrix.h"
#include "3d/Material.h"
#include "3d/DirectionLight.h"
#include "math/MathUtility.h"
#include "externals/imgui/imgui.h"
#include <cassert>
#include <format>



void GameScene::Initialize() {

	// pso ---------------------------------------------------------------

	m_pipelineState = std::make_unique<PipelineState>();
	m_rootSignature = std::make_unique<RootSignature>(DirectXCommon::GetInstance()->GetDevice());

	m_pipelineState->SetRootSignature(m_rootSignature->Get());
	m_pipelineState->SetInputLayout(VertexData::InputLayout);
	m_pipelineState->SetVS(L"Object3d.VS.hlsl", L"vs_6_0");
	m_pipelineState->SetPS(L"Object3d.PS.hlsl", L"ps_6_0");
	m_pipelineState->Create(DirectXCommon::GetInstance()->GetDevice());

	// -------------------------------------------------------------------


	// camera ------------------------------------------------------------

	m_debugCamera = std::make_unique<DebugCamera>();
	m_debugCamera->Initialize();

	// -------------------------------------------------------------------


	// buffer -------------------------------------------------------

	ID3D12Device *device = DirectXCommon::GetInstance()->GetDevice();

	// vertex
	VertexData vertices[] = {
		{ { -0.5f, -0.5f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }, // 左下
		{ { 0.0f, 0.5f, 0.0f, 1.0f }, { 0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f } }, // 上
		{ { 0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }, // 右下
	};
	m_vertexBuffer = std::make_unique<VertexBuffer>(device, 3, sizeof(VertexData), vertices);
	assert(m_vertexBuffer->IsValid());

	// index
	uint32_t indices[] = { 0, 1, 2 };
	m_indexBuffer = std::make_unique<IndexBuffer>(device, 3, indices);
	assert(m_indexBuffer->IsValid());

	// directionLight
	DirectionLight dirLightData = { { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, 1.0f };
	m_dirLightBuffer = std::make_unique<ConstBuffer>(device, sizeof(DirectionLight), &dirLightData);
	assert(m_dirLightBuffer->IsValid());

	// -------------------------------------------------------------------


	// texture -----------------------------------------------------------

	m_texHandles[0] = TextureManager::Load("monsterBall.png");
	m_texHandles[1] = TextureManager::Load("uvChecker.png");

	// -------------------------------------------------------------------

	CreateTriangle();
}


void GameScene::Update() {

	// imgui -------------------------------------------------------------

	ImGui::Begin("Settings");

	if (ImGui::Button("Create")) {
		CreateTriangle();
	}

	// obj
	for (uint32_t i = 0; i < m_triangles.size(); i++) {
		if (!ImGui::CollapsingHeader(std::format("Object {}", i).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			continue;
		}
		ImGui::PushID(i);

		ImGui::DragFloat3("Translate", &m_triangles[i].transform.translate.x, 0.01f);
		ImGui::DragFloat3("Rotate", &m_triangles[i].transform.rotate.x, 0.01f);
		ImGui::DragFloat3("Scale", &m_triangles[i].transform.scale.x, 0.01f);

		if (ImGui::Button("Delete")) {
			DeleteTriangle(i);
			ImGui:: PopID();
			i--;
			continue;
		}

		// material ------------------------------------------------------

		ImGui::Indent();
		if (!ImGui::CollapsingHeader("Material")) {
			ImGui::Unindent();
			ImGui::PopID();
			continue;
		}

		auto *materialData = m_triangles[i].materialBuffer->GetData<Material>();

		ImGui::DragFloat2("UVTranslate", &m_triangles[i].uvTransform.translate.x, 0.001f);
		ImGui::DragFloat("UVrotate", &m_triangles[i].uvTransform.rotate.z, 0.001f);
		ImGui::DragFloat2("UVScale", &m_triangles[i].uvTransform.scale.x, 0.001f);
		materialData->uvTransform = MathUtility::MakeAffineMatrix(m_triangles[i].uvTransform.scale, m_triangles[i].uvTransform.rotate, m_triangles[i].uvTransform.translate);
		ImGui::ColorEdit4("color", &materialData->color.x);
		ImGui::Combo("Lighting", &materialData->enableLighting, "None\0Apply\0");

		if (ImGui::Combo("Texture", &m_triangles[i].texIndex, "monsterBall.png\0uvChecker.png\0")) {
			m_triangles[i].texHandle = m_texHandles[m_triangles[i].texIndex];
		}
		ImGui::Unindent();

		// ---------------------------------------------------------------

		ImGui::PopID();
	}

	// light
	ImGui::Separator();
	if (ImGui::CollapsingHeader("Light")) {
		ImGui::PushID("Light");

		auto *lightData = m_dirLightBuffer->GetData<DirectionLight>();

		ImGui::ColorEdit4("color", &lightData->color.x);
		Vector3 dir = lightData->direction;
		if (ImGui::DragFloat3("direction", &dir.x, 0.01f)) {
			float len = MathUtility::Length(dir);
			if (len != 0.0f) {
				lightData->direction = MathUtility::Normalize(dir);
			}
		}
		ImGui::DragFloat("intensity", &lightData->intensity, 0.001f, 0.0f, 1.0f);

		ImGui::PopID();
	}

	ImGui::End();


	ImGui::Begin("Camera Control");
	ImGui::Text("Middle Drag        : Orbit rotation");
	ImGui::Text("Middle Drag + Shift: XY-axes movement");
	ImGui::Text("Scroll Wheel       : Z-axis movement");
	ImGui::Text("R                  : Reset transform");
	ImGui::End();

	// -------------------------------------------------------------------


	m_debugCamera->Update();

	for (auto &tri : m_triangles) {
		auto *wvpData = tri.wvpBuffer->GetData<TransformationMatrix>();

		wvpData->World = MathUtility::MakeAffineMatrix(tri.transform.scale, tri.transform.rotate, tri.transform.translate);
		wvpData->WVP = wvpData->World * m_debugCamera->GetViewProjectionMatrix();
	}
}


void GameScene::Draw() const {
	DirectXCommon *dxCommon = DirectXCommon::GetInstance();
	ID3D12GraphicsCommandList *commandList = dxCommon->GetCommandListForRender();

	assert(commandList);
	assert(m_pipelineState->IsValid());
	assert(m_rootSignature->IsValid());
	assert(m_vertexBuffer->IsValid());
	assert(m_indexBuffer->IsValid());

	// ルートシグネチャとパイプラインステートを設定
	commandList->SetGraphicsRootSignature(m_rootSignature->Get());
	commandList->SetPipelineState(m_pipelineState->Get());

	// 頂点バッファを設定
	D3D12_VERTEX_BUFFER_VIEW vbv = m_vertexBuffer->View();
	commandList->IASetVertexBuffers(0, 1, &vbv);

	// インデックスバッファを設定
	D3D12_INDEX_BUFFER_VIEW ibv = m_indexBuffer->View();
	commandList->IASetIndexBuffer(&ibv);

	// プリミティブトポロジーを設定
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 平行光源の設定
	commandList->SetGraphicsRootConstantBufferView(3, m_dirLightBuffer->GetAddress());

	// 三角形の描画
	for (const auto &tri : m_triangles) {
		commandList->SetGraphicsRootConstantBufferView(0, tri.materialBuffer->GetAddress());
		commandList->SetGraphicsRootConstantBufferView(1, tri.wvpBuffer->GetAddress());

		TextureManager::GetInstance()->SetGraphicsRootDescriptorTable(commandList, 2, tri.texHandle);

		commandList->DrawIndexedInstanced(3, 1, 0, 0, 0);
	}
}


void GameScene::CreateTriangle() {
	ID3D12Device *device = DirectXCommon::GetInstance()->GetDevice();

	assert(device);
	assert(m_vertexBuffer->IsValid());
	assert(m_indexBuffer->IsValid());

	TriangleObj triangle{};

	// wvp
	triangle.wvpBuffer = std::make_unique<ConstBuffer>(device, sizeof(TransformationMatrix));
	assert(triangle.wvpBuffer->IsValid());

	// material
	Material materialData{};
	materialData.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData.uvTransform = MathUtility::MakeIdentity4x4();
	triangle.materialBuffer = std::make_unique<ConstBuffer>(device, sizeof(Material), &materialData);
	assert(triangle.materialBuffer->IsValid());

	// texture
	triangle.texIndex = 1;
	triangle.texHandle = m_texHandles[triangle.texIndex];

	// create
	m_triangles.push_back(std::move(triangle));
}


void GameScene::DeleteTriangle(int index) {
	m_triangles.erase(m_triangles.begin() + index);
}