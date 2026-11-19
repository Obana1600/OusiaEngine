#include "Material.h"
#include "Asset/AssetDatabase.h"
#include "Diagnostics/Fatal.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"
#include "Serialize/Serializer.h"

#include <cstddef>
#include <cstring>
#include <optional>
#include <utility>

using namespace ou;


namespace {

// シェーダの値を上書きするかと、上書きする値の組（{"override": true, "value": "SRC_ALPHA"}）
template <typename E>
void OverrideEnum(Serializer &serializer, std::string_view name, std::optional<E> &value, E fallback) {
	auto isOverridden = value.has_value();
	auto content = value.value_or(fallback);
	serializer.Struct(name, [&] {
		serializer.Field("override", isOverridden);
		serializer.Enum("value", content);
	});
	if (serializer.IsReading()) { value = isOverridden ? std::optional<E>(content) : std::nullopt; }
}

void OverrideBool(Serializer &serializer, std::string_view name, std::optional<bool> &value, bool fallback) {
	auto isOverridden = value.has_value();
	auto content = value.value_or(fallback);
	serializer.Struct(name, [&] {
		serializer.Field("override", isOverridden);
		serializer.Field("value", content);
	});
	if (serializer.IsReading()) { value = isOverridden ? std::optional<bool>(content) : std::nullopt; }
}

} // namespace


void ou::Material::StaticInitialize() {
	// 既定のシェーダ
	sStandardShader_ = AssetDatabase::LoadAssetAtPath<Shader>("EngineAssets/Shader/Standard.shader");
	if (!sStandardShader_) {
		FatalError("Standard のシェーダの読み込みに失敗しました");
	}

	// 既定のテクスチャ
	sWhiteTexture_ = AssetDatabase::LoadAssetAtPath<Texture>("EngineAssets/Texture/white1x1.png");
	if (!sWhiteTexture_) {
		FatalError("マテリアルの既定のテクスチャの読み込みに失敗しました");
	}

	// エラー用のマテリアル
	auto errorShader = AssetDatabase::LoadAssetAtPath<Shader>("EngineAssets/Shader/Error.shader");
	if (!errorShader) {
		FatalError("エラー用のシェーダの読み込みに失敗しました");
	}
	sErrorMaterial_ = Create(std::move(errorShader));
	sErrorMaterial_->SetName("Error");
}


void ou::Material::StaticFinalize() {
	sErrorMaterial_ = nullptr;
	sStandardShader_ = nullptr;
	sWhiteTexture_ = nullptr;
}


std::shared_ptr<Material> ou::Material::Create(std::shared_ptr<Shader> shader) {
	auto material = std::make_shared<Material>();
	material->SetShader(shader ? std::move(shader) : sStandardShader_);
	material->SetMainTexture(sWhiteTexture_);
	return material;
}


std::shared_ptr<Material> ou::Material::Create(const Material &source) {
	auto material = std::make_shared<Material>();
	material->SetName(source.GetName());

	material->shader_ = source.shader_;
	material->mainTexture_ = source.mainTexture_;
	material->renderQueue_ = source.renderQueue_;
	material->renderStateOverride_ = source.renderStateOverride_;
	material->uvOffset_ = source.uvOffset_;
	material->uvScale_ = source.uvScale_;
	material->uvRotation_ = source.uvRotation_;
	material->color_ = source.color_;
	material->lightingMode_ = source.lightingMode_;
	material->propertyValues_ = source.propertyValues_;

	return material;
}


void ou::Material::Serialize(Serializer &serializer) {
	// シェーダを替えたら、プロパティはそのシェーダの初期値に戻す（このあと "properties" があれば読む）
	auto shader = shader_;
	serializer.Asset("shader", shader);
	if (serializer.IsReading() && shader != shader_) { SetShader(std::move(shader)); }

	serializer.Field("renderQueue", renderQueue_);
	serializer.Asset("mainTexture", mainTexture_);
	serializer.Field("color", color_);
	serializer.Field("uvOffset", uvOffset_);
	serializer.Field("uvScale", uvScale_);

	// 中はラジアン、ファイルとエディタは度
	auto uvRotationDegrees = uvRotation_ * Mathf::Rad2Deg;
	serializer.Field("uvRotation", uvRotationDegrees);
	if (serializer.IsReading()) { uvRotation_ = uvRotationDegrees * Mathf::Deg2Rad; }

	serializer.Enum("lightingMode", lightingMode_);

	serializer.Struct("renderStateOverride", [&] {
		OverrideEnum(serializer, "srcBlend", renderStateOverride_.srcBlend, Graphics::Blend::ONE);
		OverrideEnum(serializer, "dstBlend", renderStateOverride_.dstBlend, Graphics::Blend::ZERO);
		OverrideEnum(serializer, "blendOp", renderStateOverride_.blendOp, Graphics::BlendOp::ADD);
		OverrideBool(serializer, "zWrite", renderStateOverride_.zWrite, true);
		OverrideEnum(serializer, "cull", renderStateOverride_.cull, Graphics::CullMode::BACK);
	});

	if (!shader_ || shader_->GetProperties().empty()) { return; }
	serializer.Struct("properties", [&] {
		const auto &properties = shader_->GetProperties();
		for (size_t i = 0; i < properties.size() && i < propertyValues_.size(); i++) {
			const auto &property = properties[i];
			auto &value = propertyValues_[i];
			switch (property.type) {
				case Shader::PropertyType::Float:
					serializer.Field(property.name, value.x);
					break;
				case Shader::PropertyType::Vector:
					serializer.Field(property.name, value);
					break;
				case Shader::PropertyType::Color: {
					auto color = Color(value);
					serializer.Field(property.name, color);
					if (serializer.IsReading()) { value = color; }
					break;
				}
			}
		}
	});
}


Material::PerMaterial ou::Material::MakePerMaterial() const {
	PerMaterial constants{};
	constants.uvTransform = Matrix4x4::SRT(
		{uvScale_, 1.0f},
		Vector3{0.0f, 0.0f, uvRotation_},
		{uvOffset_, 0.0f});
	constants.color = color_.Linear();
	constants.lightingMode = lightingMode_;

	const auto &texture = mainTexture_ ? mainTexture_ : sWhiteTexture_;
	constants.textureIndex_ = texture->GetDescriptorIndex();

	return constants;
}


void ou::Material::SetShader(std::shared_ptr<Shader> shader) {
	shader_ = std::move(shader);

	propertyValues_.clear();
	if (shader_) {
		for (const auto &property : shader_->GetProperties()) {
			propertyValues_.push_back(property.defaultValue);
		}
	}
}


const std::shared_ptr<Shader> &ou::Material::GetShader() const {
	return shader_;
}


void ou::Material::SetRenderQueue(int renderQueue) {
	renderQueue_ = renderQueue;
}


int ou::Material::GetRenderQueue() const {
	if (renderQueue_ >= 0) { return renderQueue_; }
	return shader_ ? shader_->GetRenderQueue() : Shader::RenderQueue::Geometry;
}


void ou::Material::SetRenderStateOverride(const Shader::RenderStateOverride &renderStateOverride) {
	renderStateOverride_ = renderStateOverride;
}


const Shader::RenderStateOverride &ou::Material::GetRenderStateOverride() const {
	return renderStateOverride_;
}


void ou::Material::SetColor(const Color &color) {
	color_ = color;
}


const Color &ou::Material::GetColor() const {
	return color_;
}


void ou::Material::SetMainTexture(std::shared_ptr<Texture> mainTexture) {
	mainTexture_ = std::move(mainTexture);
}


const std::shared_ptr<Texture> &ou::Material::GetMainTexture() const {
	return mainTexture_;
}


void ou::Material::SetUvOffset(const Vector2 &uvOffset) {
	uvOffset_ = uvOffset;
}


const Vector2 &ou::Material::GetUvOffset() const {
	return uvOffset_;
}


void ou::Material::SetUvRotation(float uvRotation) {
	uvRotation_ = uvRotation;
}


float ou::Material::GetUvRotation() const {
	return uvRotation_;
}


void ou::Material::SetUvScale(const Vector2 &uvScale) {
	uvScale_ = uvScale;
}


const Vector2 &ou::Material::GetUvScale() const {
	return uvScale_;
}


void ou::Material::SetLightingMode(LightingMode lightingMode) {
	lightingMode_ = lightingMode;
}


Material::LightingMode ou::Material::GetLightingMode() const {
	return lightingMode_;
}


bool ou::Material::HasProperty(std::string_view name) const {
	return shader_ && shader_->FindPropertyIndex(name) >= 0;
}


void ou::Material::SetFloat(std::string_view name, float value) {
	const auto index = FindPropertyIndex(name, Shader::PropertyType::Float);
	if (index < 0) { return; }
	propertyValues_[index] = {value, 0.0f, 0.0f, 0.0f};
}


float ou::Material::GetFloat(std::string_view name) const {
	const auto index = FindPropertyIndex(name, Shader::PropertyType::Float);
	return (index < 0) ? 0.0f
					   : propertyValues_[index].x;
}


void ou::Material::SetVector(std::string_view name, const Vector4 &value) {
	const auto index = FindPropertyIndex(name, Shader::PropertyType::Vector);
	if (index < 0) { return; }
	propertyValues_[index] = value;
}


Vector4 ou::Material::GetVector(std::string_view name) const {
	const auto index = FindPropertyIndex(name, Shader::PropertyType::Vector);
	return (index < 0) ? Vector4::Zero
					   : propertyValues_[index];
}


void ou::Material::SetColor(std::string_view name, const Color &value) {
	const auto index = FindPropertyIndex(name, Shader::PropertyType::Color);
	if (index < 0) { return; }
	propertyValues_[index] = value;
}


Color ou::Material::GetColor(std::string_view name) const {
	const auto index = FindPropertyIndex(name, Shader::PropertyType::Color);
	return (index < 0) ? Color::Black
					   : Color(propertyValues_[index]);
}


void ou::Material::WritePropertyBuffer(void *dst) const {
	if (!shader_) { return; }

	auto *bytes = static_cast<std::byte *>(dst);
	const auto &properties = shader_->GetProperties();
	for (size_t i = 0; i < properties.size(); i++) {
		const auto &property = properties[i];
		const auto &value = propertyValues_[i];

		switch (property.type) {
			case Shader::PropertyType::Float:
				std::memcpy(bytes + property.offset, &value.x, sizeof(float));
				break;
			case Shader::PropertyType::Vector:
				std::memcpy(bytes + property.offset, &value, sizeof(Vector4));
				break;
			case Shader::PropertyType::Color: {

				const Vector4 linear = Color(value).Linear();
				std::memcpy(bytes + property.offset, &linear, sizeof(Vector4));
				break;
			}
		}
	}
}


int32_t ou::Material::FindPropertyIndex(std::string_view name, Shader::PropertyType type) const {
	const auto index = shader_ ? shader_->FindPropertyIndex(name) : -1;
	if (index < 0) {
		Log::Error("マテリアル \"{}\" のシェーダにプロパティ \"{}\" はありません", GetName(), name);
		return -1;
	}
	if (shader_->GetProperties()[index].type != type) {
		Log::Error("マテリアル \"{}\" のプロパティ \"{}\" は、この型では読み書きできません", GetName(), name);
		return -1;
	}

	return index;
}
