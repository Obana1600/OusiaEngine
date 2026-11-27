#include "LineRenderer.h"
#include "Asset/AssetDatabase.h"
#include "Diagnostics/Fatal.h"
#include "Math/Mathf.h"
#include "Render/Asset/Material.h"
#include "Scene/Transform.h"
#include "Serialize/Serializer.h"

#include <algorithm>

using namespace ou;


void ou::LineRenderer::StaticInitialize() {
	sShader_ = AssetDatabase::LoadAssetAtPath<Shader>("EngineAssets/Shader/Line.shader");
	if (!sShader_) {
		FatalError("線のシェーダの読み込みに失敗しました");
	}
	sDefaultMaterial_ = Material::Create(sShader_);
	sDefaultMaterial_->SetName("Default-Line");

	sWhiteTexture_ = AssetDatabase::LoadAssetAtPath<Texture>("EngineAssets/Texture/white1x1.png");
	if (!sWhiteTexture_) {
		FatalError("線の既定のテクスチャの読み込みに失敗しました");
	}
}


void ou::LineRenderer::StaticFinalize() {
	sShader_ = nullptr;
	sDefaultMaterial_ = nullptr;
	sWhiteTexture_ = nullptr;
}


void ou::LineRenderer::Awake() {
	Renderer::Awake();

	if (!GetSharedMaterial()) {
		SetSharedMaterial(sDefaultMaterial_);
	}
}


void ou::LineRenderer::LateUpdate() {
	const auto count = static_cast<uint32_t>(positions_.size());
	if (count < 2) {
		pointCount_ = 0;
		return;
	}

	worldPositions_.clear();
	const auto &localToWorld = GetTransform()->GetLocalToWorldMatrix();
	for (const auto &position : positions_) {
		worldPositions_.push_back(useWorldSpace_ ? position : localToWorld.MultiplyPoint3x4(position));
	}
	if (loop_) {
		worldPositions_.push_back(worldPositions_.front());
	}

	pointCount_ = static_cast<uint32_t>(worldPositions_.size());
	distances_.resize(pointCount_);
	distances_[0] = 0.0f;
	for (uint32_t i = 1; i < pointCount_; i++) {
		distances_[i] = distances_[i - 1] + Vector3::Distance(worldPositions_[i - 1], worldPositions_[i]);
	}
	const auto totalLength = distances_.back();

	EnsureCapacity(pointCount_);

	auto &buffer = pointBuffers_[Graphics::GetDevice()->GetBufferIndex()];
	auto *data = static_cast<PointData *>(buffer.mappedData);
	for (uint32_t i = 0; i < pointCount_; i++) {
		const auto t = totalLength > 0.0f ? distances_[i] / totalLength : 0.0f;
		data[i].position = worldPositions_[i];
		data[i].width = Mathf::Lerp(startWidth_, endWidth_, t) * widthMultiplier_;
		data[i].color = Color::Lerp(startColor_, endColor_, t).Linear();
		data[i].texcoord = t;
	}
}


void ou::LineRenderer::Serialize(Serializer &serializer) {
	Renderer::Serialize(serializer);

	serializer.Array("positions", positions_, [&](Vector3 &position) {
		serializer.Field("", position);
	});
	serializer.Field("startWidth", startWidth_);
	serializer.Field("endWidth", endWidth_);
	serializer.Field("widthMultiplier", widthMultiplier_);
	serializer.Field("startColor", startColor_);
	serializer.Field("endColor", endColor_);
	serializer.Field("useWorldSpace", useWorldSpace_);
	serializer.Field("loop", loop_);
}


void ou::LineRenderer::Draw(std::string_view lightMode, Graphics::CommandList cmd) const {
	if (pointCount_ < 2) { return; }

	const auto &material = GetSharedMaterial();
	if (!material || !material->GetShader()) { return; }

	auto *device = Graphics::GetDevice();
	const auto &texture = material->GetMainTexture() ? material->GetMainTexture() : sWhiteTexture_;
	const auto &buffer = pointBuffers_[device->GetBufferIndex()];

	PerLineRenderer constants{};
	constants.pointBufferIndex = static_cast<uint32_t>(device->GetDescriptorIndex(&buffer, Graphics::ViewType::SRV));
	constants.pointCount = pointCount_;
	constants.textureIndex = texture->GetDescriptorIndex();
	constants.loop = loop_ ? 1u : 0u;
	device->BindDynamicConstantBuffer(constants, 0, cmd);

	const auto &shader = material->GetShader();
	const auto &passes = shader->GetPasses();
	for (size_t pass = 0; pass < passes.size(); pass++) {
		if (passes[pass].lightMode != lightMode) { continue; }

		const auto *pso = shader->GetPipelineState(
			pass, material->GetRenderStateOverride(), nullptr, Graphics::PrimitiveTopology::TRIANGLESTRIP);
		if (!pso) { continue; }

		device->BindPipelineState(pso, cmd);
		device->Draw(pointCount_ * 2, 0, cmd);
	}
}


void ou::LineRenderer::SetPositionCount(uint32_t count) {
	positions_.resize(count, Vector3::Zero);
}


uint32_t ou::LineRenderer::GetPositionCount() const {
	return static_cast<uint32_t>(positions_.size());
}


void ou::LineRenderer::SetPosition(uint32_t index, const Vector3 &position) {
	if (index >= positions_.size()) { return; }
	positions_[index] = position;
}


Vector3 ou::LineRenderer::GetPosition(uint32_t index) const {
	if (index >= positions_.size()) { return Vector3::Zero; }
	return positions_[index];
}


void ou::LineRenderer::SetPositions(std::span<const Vector3> positions) {
	positions_.assign(positions.begin(), positions.end());
}


const std::vector<Vector3> &ou::LineRenderer::GetPositions() const {
	return positions_;
}


void ou::LineRenderer::SetStartWidth(float width) {
	startWidth_ = width;
}


float ou::LineRenderer::GetStartWidth() const {
	return startWidth_;
}


void ou::LineRenderer::SetEndWidth(float width) {
	endWidth_ = width;
}


float ou::LineRenderer::GetEndWidth() const {
	return endWidth_;
}


void ou::LineRenderer::SetWidthMultiplier(float widthMultiplier) {
	widthMultiplier_ = widthMultiplier;
}


float ou::LineRenderer::GetWidthMultiplier() const {
	return widthMultiplier_;
}


void ou::LineRenderer::SetStartColor(const Color &color) {
	startColor_ = color;
}


const Color &ou::LineRenderer::GetStartColor() const {
	return startColor_;
}


void ou::LineRenderer::SetEndColor(const Color &color) {
	endColor_ = color;
}


const Color &ou::LineRenderer::GetEndColor() const {
	return endColor_;
}


void ou::LineRenderer::SetUseWorldSpace(bool useWorldSpace) {
	useWorldSpace_ = useWorldSpace;
}


bool ou::LineRenderer::GetUseWorldSpace() const {
	return useWorldSpace_;
}


void ou::LineRenderer::SetLoop(bool loop) {
	loop_ = loop;
}


bool ou::LineRenderer::GetLoop() const {
	return loop_;
}


void ou::LineRenderer::EnsureCapacity(uint32_t count) {
	if (count <= capacity_) { return; }

	capacity_ = std::max({count, capacity_ * 2, 64u});

	Graphics::GpuBufferDesc desc{};
	desc.size = capacity_ * sizeof(PointData);
	desc.heapType = Graphics::HeapType::UPLOAD;
	desc.bindFlags = Graphics::BindFlag::SHADER_RESOURCE;
	desc.miscFlags = Graphics::ResourceMiscFlag::BUFFER_STRUCTURED;
	desc.stride = sizeof(PointData);

	for (auto &buffer : pointBuffers_) {
		if (!Graphics::GetDevice()->CreateBuffer(&desc, nullptr, &buffer)) {
			FatalError("線のバッファの作成に失敗しました");
		}
	}
}
