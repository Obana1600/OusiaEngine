#include "ParticleSystemRenderer.h"
#include "Asset/AssetDatabase.h"
#include "Diagnostics/Fatal.h"
#include "Particle/ParticleSystem.h"
#include "Render/Camera.h"
#include "Render/Material.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"

#include <algorithm>
#include <numeric>

using namespace ou;


namespace {

void ApplyBlendMode(
	ParticleSystemRenderer::BlendMode blendMode, Shader::RenderStateOverride &override) {
	switch (blendMode) {
		case ParticleSystemRenderer::BlendMode::Additive:
			override.srcBlend = Graphics::Blend::SRC_ALPHA;
			override.dstBlend = Graphics::Blend::ONE;
			override.blendOp = Graphics::BlendOp::ADD;
			break;

		case ParticleSystemRenderer::BlendMode::Subtract:
			override.srcBlend = Graphics::Blend::SRC_ALPHA;
			override.dstBlend = Graphics::Blend::ONE;
			override.blendOp = Graphics::BlendOp::REV_SUBTRACT;
			break;

		case ParticleSystemRenderer::BlendMode::Multiply:
			override.srcBlend = Graphics::Blend::ZERO;
			override.dstBlend = Graphics::Blend::SRC_COLOR;
			override.blendOp = Graphics::BlendOp::ADD;
			break;

		case ParticleSystemRenderer::BlendMode::Alpha:
			override.srcBlend = Graphics::Blend::SRC_ALPHA;
			override.dstBlend = Graphics::Blend::INV_SRC_ALPHA;
			override.blendOp = Graphics::BlendOp::ADD;
			break;

		default:
			break;
	}
}

} // namespace



void ou::ParticleSystemRenderer::StaticInitialize() {
	sShader_ = AssetDatabase::LoadAssetAtPath<Shader>("EngineAssets/Shader/Particle.shader");
	if (!sShader_) {
		FatalError("パーティクルのシェーダの読み込みに失敗しました");
	}
	sDefaultMaterial_ = Material::Create(sShader_);
	sDefaultMaterial_->SetName("Default-ParticleSystem");

	sWhiteTexture_ = AssetDatabase::LoadAssetAtPath<Texture>("EngineAssets/Texture/white1x1.png");
	if (!sWhiteTexture_) {
		FatalError("パーティクルの既定のテクスチャの読み込みに失敗しました");
	}
}


void ou::ParticleSystemRenderer::StaticFinalize() {
	sShader_ = nullptr;
	sDefaultMaterial_ = nullptr;
	sWhiteTexture_ = nullptr;
}


void ou::ParticleSystemRenderer::Awake() {
	Renderer::Awake();

	// ParticleSystem がなければつける
	particleSystem_ = GetGameObject()->GetComponent<ParticleSystem>();
	if (!particleSystem_) {
		particleSystem_ = GetGameObject()->AddComponent<ParticleSystem>();
	}

	// マテリアルがなければ新しく作る
	if (!GetSharedMaterial()) {
		SetSharedMaterial(sDefaultMaterial_);
	}
}


void ou::ParticleSystemRenderer::LateUpdate() {
	const auto &particles = particleSystem_->GetParticles();
	particleCount_ = static_cast<uint32_t>(particles.size());
	if (particleCount_ == 0) { return; }

	EnsureCapacity(particleCount_);

	// 描画順を決める
	drawOrder_.resize(particleCount_);
	std::iota(drawOrder_.begin(), drawOrder_.end(), 0u);

	switch (sortMode_) {
		case SortMode::None:
		case SortMode::YoungestInFront:
			break;

		case SortMode::OldestInFront:
			std::reverse(drawOrder_.begin(), drawOrder_.end());
			break;

		case SortMode::Distance:
			SortByDistance(particles);
			break;
	}

	// 現在フレームのバッファに書く
	auto &buffer = particleBuffers_[Graphics::GetDevice()->GetBufferIndex()];
	auto *data = static_cast<ParticleData *>(buffer.mappedData);
	for (uint32_t i = 0; i < particleCount_; i++) {
		const auto &particle = particles[drawOrder_[i]];
		data[i].position = particle.position;
		data[i].size = particleSystem_->EvaluateSize(particle);
		data[i].color = particleSystem_->EvaluateColor(particle).Linear();
	}
}


void ou::ParticleSystemRenderer::Draw(Graphics::CommandList cmd) const {
	if (particleCount_ == 0) { return; }

	const auto &material = GetSharedMaterial();
	if (!material || !material->GetShader()) { return; }

	auto *device = Graphics::GetDevice();
	const auto isLocal = particleSystem_->GetMain().simulationSpace == ParticleSystem::SimulationSpace::Local;
	const auto &texture = material->GetMainTexture() ? material->GetMainTexture() : sWhiteTexture_;
	const auto &buffer = particleBuffers_[device->GetBufferIndex()];

	PerParticleSystem constants{};
	constants.world = isLocal ? GetTransform()->GetLocalToWorldMatrix() : Matrix4x4::Identity;
	constants.particleBufferIndex = static_cast<uint32_t>(device->GetDescriptorIndex(&buffer, Graphics::ViewType::SRV));
	constants.textureIndex = texture->GetDescriptorIndex();
	constants.blendMode = static_cast<uint32_t>(blendMode_);
	device->BindDynamicConstantBuffer(constants, 0, cmd);

	// Pass だけ順に描く
	const auto &shader = material->GetShader();
	for (size_t pass = 0; pass < shader->GetPassCount(); pass++) {
		const auto *pso = shader->GetPipelineState(
			pass, material->GetRenderStateOverride(), nullptr, Graphics::PrimitiveTopology::TRIANGLESTRIP);
		if (!pso) { continue; }

		device->BindPipelineState(pso, cmd);
		device->DrawInstanced(4, particleCount_, 0, 0, cmd);
	}
}


void ou::ParticleSystemRenderer::SetBlendMode(BlendMode blendMode) {
	blendMode_ = blendMode;

	const auto &material = GetMaterial();
	if (!material) { return; }

	auto override = material->GetRenderStateOverride();
	ApplyBlendMode(blendMode, override);
	material->SetRenderStateOverride(override);
}


ParticleSystemRenderer::BlendMode ou::ParticleSystemRenderer::GetBlendMode() const {
	return blendMode_;
}


void ou::ParticleSystemRenderer::SetSortMode(SortMode sortMode) {
	sortMode_ = sortMode;
}


ParticleSystemRenderer::SortMode ou::ParticleSystemRenderer::GetSortMode() const {
	return sortMode_;
}


void ou::ParticleSystemRenderer::EnsureCapacity(uint32_t count) {
	if (count <= capacity_) { return; }

	capacity_ = std::max({count, capacity_ * 2, 64u});

	Graphics::GpuBufferDesc desc{};
	desc.size = capacity_ * sizeof(ParticleData);
	desc.heapType = Graphics::HeapType::UPLOAD;
	desc.bindFlags = Graphics::BindFlag::SHADER_RESOURCE;
	desc.miscFlags = Graphics::ResourceMiscFlag::BUFFER_STRUCTURED;
	desc.stride = sizeof(ParticleData);

	auto *device = Graphics::GetDevice();
	for (auto &buffer : particleBuffers_) {
		if (!device->CreateBuffer(&desc, nullptr, &buffer)) {
			FatalError("パーティクルのバッファの作成に失敗しました");
		}
	}
}


void ou::ParticleSystemRenderer::SortByDistance(const std::vector<ParticleSystem::Particle> &particles) {
	const auto *camera = GetGameObject()->GetScene()->GetMainCamera();
	if (!camera) { return; }

	const auto cameraPosition = camera->GetTransform()->GetWorldPosition();
	const auto isLocal = particleSystem_->GetMain().simulationSpace == ParticleSystem::SimulationSpace::Local;
	const auto &localToWorld = GetTransform()->GetLocalToWorldMatrix();

	sortDistances_.resize(particles.size());
	for (size_t i = 0; i < particles.size(); i++) {
		const auto &position = particles[i].position;
		const auto worldPosition = isLocal ? localToWorld.MultiplyPoint3x4(position) : position;
		const auto toCamera = worldPosition - cameraPosition;
		sortDistances_[i] = Vector3::Dot(toCamera, toCamera);
	}

	std::sort(drawOrder_.begin(), drawOrder_.end(), [this](uint32_t a, uint32_t b) {
		return sortDistances_[a] > sortDistances_[b];
	});
}
