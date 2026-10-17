#include "ParticleSystemRenderer.h"
#include "Asset/AssetDatabase.h"
#include "Core/Fatal.h"
#include "Math/Mathf.h"
#include "Particle/ParticleSystem.h"
#include "Scene/Camera.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"

#include <algorithm>
#include <numeric>
#include <utility>

using namespace ou;


namespace {

Graphics::BlendState MakeBlendState(ParticleSystemRenderer::BlendMode blendMode) {
	Graphics::BlendState blend{};
	auto &target = blend.renderTarget[0];
	target.blendEnable = true;

	switch (blendMode) {
		case ParticleSystemRenderer::BlendMode::Additive:
			target.srcBlend = Graphics::Blend::SRC_ALPHA;
			target.destBlend = Graphics::Blend::ONE;
			break;

		case ParticleSystemRenderer::BlendMode::Subtract:
			target.srcBlend = Graphics::Blend::SRC_ALPHA;
			target.destBlend = Graphics::Blend::ONE;
			target.blendOp = Graphics::BlendOp::REV_SUBTRACT;
			break;

		case ParticleSystemRenderer::BlendMode::Multiply:
			target.srcBlend = Graphics::Blend::ZERO;
			target.destBlend = Graphics::Blend::SRC_COLOR;
			break;

		case ParticleSystemRenderer::BlendMode::Alpha:
			target.srcBlend = Graphics::Blend::SRC_ALPHA;
			target.destBlend = Graphics::Blend::INV_SRC_ALPHA;
			break;

		default:
			break;
	}

	return blend;
}

Graphics::PipelineStateDesc MakePsoDesc(
	const Graphics::Shader *vs,
	const Graphics::Shader *ps,
	const Graphics::BlendState *blend,
	const Graphics::DepthStencilState *depthStencil) {
	Graphics::PipelineStateDesc desc{};
	desc.vs = vs;
	desc.ps = ps;
	desc.bs = blend;
	desc.dss = depthStencil;
	desc.pt = Graphics::PrimitiveTopology::TRIANGLESTRIP;
	return desc;
}

} // namespace



void ou::ParticleSystemRenderer::StaticInitialize(const Graphics::RenderPassInfo &renderPass) {
	sVs_ = AssetDatabase::LoadAssetAtPath<Shader>("EngineAssets/Shader/Particle.VS.hlsl");
	sPs_ = AssetDatabase::LoadAssetAtPath<Shader>("EngineAssets/Shader/Particle.PS.hlsl");
	if (!sVs_ || !sPs_) {
		FatalError("パーティクルのシェーダの作成に失敗しました");
	}

	sWhiteTexture_ = AssetDatabase::LoadAssetAtPath<Texture>("EngineAssets/Texture/white1x1.png");
	if (!sWhiteTexture_) {
		FatalError("パーティクルの既定のテクスチャの読み込みに失敗しました");
	}

	// 深度ステンシル
	Graphics::DepthStencilState depthStencil{};
	depthStencil.depthEnable = true;
	depthStencil.depthWriteMask = Graphics::DepthWriteMask::ZERO;
	depthStencil.comparisonFunc = Graphics::ComparisonFunc::LESS_EQUAL;

	auto *device = Graphics::GetDevice();
	const auto *vs = &sVs_->GetGraphicsShader();
	const auto *ps = &sPs_->GetGraphicsShader();

	for (size_t i = 0; i < static_cast<size_t>(BlendMode::Count); i++) {
		const auto blend = MakeBlendState(static_cast<BlendMode>(i));
		const auto desc = MakePsoDesc(vs, ps, &blend, &depthStencil);
		if (!device->CreatePipelineState(&desc, &sPsos_[i], &renderPass)) {
			FatalError("パーティクルの PSO の作成に失敗しました");
		}
	}
}


void ou::ParticleSystemRenderer::StaticFinalize() {
	for (auto &pso : sPsos_) {
		pso = {};
	}
	sWhiteTexture_ = nullptr;
	sVs_ = nullptr;
	sPs_ = nullptr;
}


void ou::ParticleSystemRenderer::Awake() {
	Renderer::Awake();

	// ParticleSystem がなければつける
	particleSystem_ = GetGameObject()->GetComponent<ParticleSystem>();
	if (!particleSystem_) {
		particleSystem_ = GetGameObject()->AddComponent<ParticleSystem>();
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

	auto *device = Graphics::GetDevice();
	device->BindPipelineState(&sPsos_[static_cast<size_t>(blendMode_)], cmd);

	const auto isLocal = particleSystem_->GetMain().simulationSpace == ParticleSystem::SimulationSpace::Local;
	const auto &texture = texture_ ? texture_ : sWhiteTexture_;
	const auto &buffer = particleBuffers_[device->GetBufferIndex()];

	PerParticleSystem constants{};
	constants.world = isLocal ? GetTransform()->GetLocalToWorldMatrix() : Matrix4x4::Identity;
	constants.particleBufferIndex = static_cast<uint32_t>(device->GetDescriptorIndex(&buffer, Graphics::ViewType::SRV));
	constants.textureIndex = texture->GetDescriptorIndex();
	constants.blendMode = static_cast<uint32_t>(blendMode_);
	device->BindDynamicConstantBuffer(constants, 0, cmd);

	device->DrawInstanced(4, particleCount_, 0, 0, cmd);
}


int ou::ParticleSystemRenderer::GetRenderQueue() const {
	return 3000;
}


void ou::ParticleSystemRenderer::SetTexture(std::shared_ptr<Texture> texture) {
	texture_ = std::move(texture);
}


const std::shared_ptr<Texture> &ou::ParticleSystemRenderer::GetTexture() const {
	return texture_;
}


void ou::ParticleSystemRenderer::SetBlendMode(BlendMode blendMode) {
	blendMode_ = blendMode;
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
