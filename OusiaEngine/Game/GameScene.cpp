#include "GameScene.h"
#include "Asset/AssetDatabase.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"
#include "Particle/ParticleSystemRenderer.h"
#include "Render/Camera.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"
#include "Serialize/TypeRegistry.h"

#include <utility>

#include <imgui/imgui.h>

using namespace ou;


namespace {

constexpr const char *kSettingsWindow = "Settings";

constexpr uint32_t kParticleBlendModeMultiply = 2;

} // namespace



void ou::BuildGameScene(Scene &scene) {
	static bool isRegistered = false;
	if (!std::exchange(isRegistered, true)) {
		TypeRegistry::Register<CameraController>("CameraController");
		TypeRegistry::Register<InstancedPlanes>("InstancedPlanes");
	}

	auto *camera = scene.CreateGameObject("Main Camera");
	camera->AddComponent<Camera>();
	camera->AddComponent<CameraController>();
	camera->GetTransform()->SetLocalPosition({0.5f, 2.5f, -7.0f});
	camera->GetTransform()->SetLocalEulerAngles(Vector3{17.0f, 0.0f, 0.0f} * Mathf::Deg2Rad);

	auto *planes = scene.CreateGameObject("Instanced Planes");
	planes->AddComponent<InstancedPlanes>();
}


void ou::CameraController::Update() {
	auto *transform = GetTransform();

	ImGui::SetNextWindowSize({400.0f, 0.0f}, ImGuiCond_FirstUseEver);
	ImGui::Begin(kSettingsWindow);
	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
		auto position = transform->GetLocalPosition();
		if (ImGui::DragFloat3("CameraTranslate", &position.x, 0.01f)) {
			transform->SetLocalPosition(position);
		}

		auto rotation = transform->GetLocalEulerAngles() * Mathf::Rad2Deg;
		if (ImGui::DragFloat3("CameraRotate", &rotation.x, 0.5f)) {
			transform->SetLocalEulerAngles(rotation * Mathf::Deg2Rad);
		}
	}
	ImGui::End();
}


void ou::InstancedPlanes::Awake() {
	Renderer::Awake();

	shader_ = AssetDatabase::LoadAssetAtPath<Shader>("EngineAssets/Shader/Particle.shader");
	texture_ = AssetDatabase::LoadAssetAtPath<Texture>("Assets/uvChecker.png");
	if (!shader_ || !texture_) {
		Log::Error("板ポリのシェーダ、もしくはテクスチャの読み込みに失敗しました");
	}

	for (uint32_t i = 0; i < kInstanceCount; i++) {
		planes_[i].translate = Vector3::One * (0.1f * static_cast<float>(i));
	}

	Graphics::GpuBufferDesc desc{};
	desc.size = kInstanceCount * sizeof(ParticleSystemRenderer::ParticleData);
	desc.heapType = Graphics::HeapType::UPLOAD;
	desc.bindFlags = Graphics::BindFlag::SHADER_RESOURCE;
	desc.miscFlags = Graphics::ResourceMiscFlag::BUFFER_STRUCTURED;
	desc.stride = sizeof(ParticleSystemRenderer::ParticleData);

	for (auto &buffer : instanceBuffers_) {
		if (!Graphics::GetDevice()->CreateBuffer(&desc, nullptr, &buffer)) {
			Log::Error("インスタンスのバッファの作成に失敗しました");
		}
	}
}


void ou::InstancedPlanes::Update() {
	ImGui::Begin(kSettingsWindow);
	if (ImGui::CollapsingHeader("Planes", ImGuiTreeNodeFlags_DefaultOpen)) {
		static constexpr const char *kBlendModes[] = {"None", "Normal", "Add", "Subtract", "Multiply", "Screen"};
		auto blendMode = static_cast<int>(blendMode_);
		if (ImGui::Combo("Blend", &blendMode, kBlendModes, IM_ARRAYSIZE(kBlendModes))) {
			blendMode_ = static_cast<BlendMode>(blendMode);
		}

		for (uint32_t i = 0; i < kInstanceCount; i++) {
			ImGui::PushID(static_cast<int>(i));
			if (ImGui::TreeNode("Plane", "Plane %u", i)) {
				auto &plane = planes_[i];
				ImGui::DragFloat3("Translate", &plane.translate.x, 0.01f);
				ImGui::DragFloat("Size", &plane.size, 0.01f, 0.0f, 100.0f);
				ImGui::ColorEdit4("Color", &plane.color.r);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}
	ImGui::End();
}


void ou::InstancedPlanes::Draw(Graphics::CommandList cmd) const {
	if (!shader_ || !texture_) { return; }

	auto *device = Graphics::GetDevice();
	const auto &buffer = instanceBuffers_[device->GetBufferIndex()];
	if (!buffer.mappedData) { return; }

	auto *instances = static_cast<ParticleSystemRenderer::ParticleData *>(buffer.mappedData);
	for (uint32_t i = 0; i < kInstanceCount; i++) {
		const auto &plane = planes_[i];
		instances[i].position = plane.translate;
		instances[i].size = plane.size;
		instances[i].color = plane.color.Linear();
	}

	ParticleSystemRenderer::PerParticleSystem constants{};
	constants.world = GetTransform()->GetLocalToWorldMatrix();
	constants.particleBufferIndex = static_cast<uint32_t>(device->GetDescriptorIndex(&buffer, Graphics::ViewType::SRV));
	constants.textureIndex = texture_->GetDescriptorIndex();
	constants.blendMode = blendMode_ == BlendMode::Multiply ? kParticleBlendModeMultiply : 0;
	device->BindDynamicConstantBuffer(constants, 0, cmd);

	const auto *pso = shader_->GetPipelineState(0, MakeRenderState(), nullptr, Graphics::PrimitiveTopology::TRIANGLESTRIP);
	if (!pso) { return; }

	device->BindPipelineState(pso, cmd);
	device->DrawInstanced(4, kInstanceCount, 0, 0, cmd);
}


Shader::RenderStateOverride ou::InstancedPlanes::MakeRenderState() const {
	using Graphics::Blend;
	using Graphics::BlendOp;

	Shader::RenderStateOverride state{};
	state.blendOp = BlendOp::ADD;
	state.zWrite = blendMode_ == BlendMode::None;

	switch (blendMode_) {
		case BlendMode::None:
			state.srcBlend = Blend::ONE;
			state.dstBlend = Blend::ZERO;
			break;
		case BlendMode::Normal:
			state.srcBlend = Blend::SRC_ALPHA;
			state.dstBlend = Blend::INV_SRC_ALPHA;
			break;
		case BlendMode::Add:
			state.srcBlend = Blend::SRC_ALPHA;
			state.dstBlend = Blend::ONE;
			break;
		case BlendMode::Subtract:
			state.srcBlend = Blend::SRC_ALPHA;
			state.dstBlend = Blend::ONE;
			state.blendOp = BlendOp::REV_SUBTRACT;
			break;
		case BlendMode::Multiply:
			state.srcBlend = Blend::ZERO;
			state.dstBlend = Blend::SRC_COLOR;
			break;
		case BlendMode::Screen:
			state.srcBlend = Blend::INV_DEST_COLOR;
			state.dstBlend = Blend::ONE;
			break;
	}
	return state;
}
