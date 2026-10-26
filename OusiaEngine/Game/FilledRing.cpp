#include "FilledRing.h"
#include "Math/Mathf.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Mesh.h"
#include "Render/MeshRenderer.h"
#include "Scene/GameObject.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace ou;


namespace {

// Origin360 の向き（+x から反時計回りの角度）
float OriginAngle(FilledRing::Origin360 origin) {
	switch (origin) {
		case FilledRing::Origin360::Bottom: return -0.5f * Mathf::PI;
		case FilledRing::Origin360::Right: return 0.0f;
		case FilledRing::Origin360::Top: return 0.5f * Mathf::PI;
		case FilledRing::Origin360::Left: return Mathf::PI;
	}
	return 0.0f;
}

} // namespace



void ou::FilledRing::Awake() {
	meshRenderer_ = GetGameObject()->GetComponent<MeshRenderer>();
	if (!meshRenderer_) {
		meshRenderer_ = GetGameObject()->AddComponent<MeshRenderer>();
	}

	// 半透明で、ライトの影響を受けず、裏からも見える
	material_ = Material::Create();
	material_->SetName("FilledRing");
	material_->SetLightingMode(Material::LightingMode::None);
	material_->SetRenderQueue(Shader::RenderQueue::Transparent);
	material_->SetColor(color_);

	Shader::RenderStateOverride override{};
	override.srcBlend = Graphics::Blend::SRC_ALPHA;
	override.dstBlend = Graphics::Blend::INV_SRC_ALPHA;
	override.blendOp = Graphics::BlendOp::ADD;
	override.zWrite = false;
	override.cull = Graphics::CullMode::NONE;
	material_->SetRenderStateOverride(override);

	meshRenderer_->SetSharedMaterial(material_);
}


void ou::FilledRing::LateUpdate() {
	if (!isDirty_) { return; }
	isDirty_ = false;
	RebuildMesh();
}


void ou::FilledRing::SetFillAmount(float fillAmount) {
	fillAmount = std::clamp(fillAmount, 0.0f, 1.0f);
	if (fillAmount_ == fillAmount) { return; }
	fillAmount_ = fillAmount;
	isDirty_ = true;
}


float ou::FilledRing::GetFillAmount() const {
	return fillAmount_;
}


void ou::FilledRing::SetFillClockwise(bool fillClockwise) {
	fillClockwise_ = fillClockwise;
	isDirty_ = true;
}


bool ou::FilledRing::GetFillClockwise() const {
	return fillClockwise_;
}


void ou::FilledRing::SetFillOrigin(Origin360 fillOrigin) {
	fillOrigin_ = fillOrigin;
	isDirty_ = true;
}


FilledRing::Origin360 ou::FilledRing::GetFillOrigin() const {
	return fillOrigin_;
}


void ou::FilledRing::SetInnerRadius(float innerRadius) {
	innerRadius_ = innerRadius;
	isDirty_ = true;
}


float ou::FilledRing::GetInnerRadius() const {
	return innerRadius_;
}


void ou::FilledRing::SetOuterRadius(float outerRadius) {
	outerRadius_ = outerRadius;
	isDirty_ = true;
}


float ou::FilledRing::GetOuterRadius() const {
	return outerRadius_;
}


void ou::FilledRing::SetColor(const Color &color) {
	color_ = color;
	if (material_) {
		material_->SetColor(color);
	}
}


const Color &ou::FilledRing::GetColor() const {
	return color_;
}


void ou::FilledRing::SetSegments(uint32_t segments) {
	segments_ = std::max(segments, 3u);
	isDirty_ = true;
}


uint32_t ou::FilledRing::GetSegments() const {
	return segments_;
}


void ou::FilledRing::RebuildMesh() {
	// 空のメッシュは作れないので、描かないときは外すだけにする
	if (fillAmount_ <= 0.0f) {
		meshRenderer_->SetMesh(nullptr);
		return;
	}

	// 満ちている角度に合わせて分割数を減らす（最低 1）
	const auto steps = std::max(1u, static_cast<uint32_t>(std::ceil(static_cast<float>(segments_) * fillAmount_)));
	const auto sweep = 2.0f * Mathf::PI * fillAmount_ * (fillClockwise_ ? -1.0f : 1.0f);
	const auto startAngle = OriginAngle(fillOrigin_);

	// 内側と外側の頂点を交互に並べる。法線はカメラ（-z）を向ける
	std::vector<Mesh::Vertex> vertices;
	vertices.reserve((steps + 1) * 2);
	for (uint32_t i = 0; i <= steps; i++) {
		const auto t = static_cast<float>(i) / static_cast<float>(steps);
		const auto angle = startAngle + sweep * t;
		const Vector3 direction = {std::cos(angle), std::sin(angle), 0.0f};
		vertices.push_back({direction * innerRadius_, Vector3::Back, {t, 1.0f}});
		vertices.push_back({direction * outerRadius_, Vector3::Back, {t, 0.0f}});
	}

	std::vector<uint32_t> indices;
	indices.reserve(steps * 6);
	for (uint32_t i = 0; i < steps; i++) {
		const auto inner = i * 2;
		const auto outer = inner + 1;
		indices.insert(indices.end(), {inner, outer, outer + 2, inner, outer + 2, inner + 2});
	}

	// 頂点の数が変わるので、毎回新しいメッシュにする（古いバッファはエンジンが GPU の使用後に手放す）
	auto mesh = std::make_shared<Mesh>();
	mesh->SetName("FilledRing");
	mesh->SetVertices(std::move(vertices));
	mesh->SetIndices(std::move(indices));
	mesh->Upload();
	meshRenderer_->SetMesh(std::move(mesh));
}
