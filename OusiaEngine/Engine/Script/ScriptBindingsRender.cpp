#include "ScriptBindings.h"
#include "Animation/Core/WrapMode.h"
#include "Animation/Skeletal/SkeletalAnimationClip.h"
#include "Animation/Skeletal/SkeletalAnimator.h"
#include "Math/Bounds.h"
#include "Math/Color.h"
#include "Math/Vector3.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleSystemRenderer.h"
#include "Render/Asset/Mesh.h"
#include "Render/Asset/Shader.h"
#include "Render/Camera.h"
#include "Render/Light.h"
#include "Render/LineRenderer.h"
#include "Render/SkinnedMeshRenderer.h"
#include "Scene/SceneManager.h"
#include "Script/ScriptBindingsInternal.h"
#include "Render/Asset/ComputeShader.h"
#include "Render/Asset/Texture.h"

#include <algorithm>
#include <cstdint>
#include <format>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include <angelscript.h>

using namespace ou;
using namespace ou::ScriptBindingsInternal;


namespace {

// ---- Camera ----

ObjectHandle *GetMainCamera() {
	return MakeHandle(SceneManager::GetMainCamera());
}


// ---- Mesh・Shader・SkeletalAnimationClip（アセット） ----

int GetVertexCount(ObjectHandle *self) {
	auto *mesh = Resolve<Mesh>(self);
	return mesh ? static_cast<int>(mesh->GetVertices().size()) : 0;
}

int GetSubMeshCount(ObjectHandle *self) {
	auto *mesh = Resolve<Mesh>(self);
	return mesh ? static_cast<int>(mesh->GetSubMeshes().size()) : 0;
}

int GetPassCount(ObjectHandle *self) {
	auto *shader = Resolve<Shader>(self);
	return shader ? static_cast<int>(shader->GetPassCount()) : 0;
}

// Unity と同じく、なければ -1
int FindPropertyIndex(const std::string &propertyName, ObjectHandle *self) {
	auto *shader = Resolve<Shader>(self);
	return shader ? static_cast<int>(shader->FindPropertyIndex(propertyName)) : -1;
}


// ---- ComputeShader ----

// Unity と同じく、ないカーネルは例外にする
int FindKernel(const std::string &name, ObjectHandle *self) {
	auto *shader = Resolve<ComputeShader>(self);
	if (!shader) { return -1; }
	if (!shader->HasKernel(name)) {
		ThrowException(std::format("ArgumentException: ComputeShader \"{}\" にカーネル \"{}\" はありません", shader->GetName(), name));
		return -1;
	}
	return shader->FindKernel(name);
}

bool HasKernel(const std::string &name, ObjectHandle *self) {
	auto *shader = Resolve<ComputeShader>(self);
	return shader && shader->HasKernel(name);
}

void GetKernelThreadGroupSizes(int kernelIndex, uint32_t &x, uint32_t &y, uint32_t &z, ObjectHandle *self) {
	x = y = z = 0;
	if (auto *shader = Resolve<ComputeShader>(self)) { shader->GetKernelThreadGroupSizes(kernelIndex, x, y, z); }
}

// SetFloat・SetInt・SetBool・SetVector・SetMatrix
template <typename V, void (ComputeShader::*Setter)(std::string_view, V)>
void SetComputeValue(const std::string &name, V value, ObjectHandle *self) {
	if (auto *shader = Resolve<ComputeShader>(self)) { (shader->*Setter)(name, value); }
}

void SetComputeVectorArray(const std::string &name, CScriptArray *values, ObjectHandle *self) {
	auto *shader = Resolve<ComputeShader>(self);
	if (!shader) { return; }
	if (!values) {
		ThrowException("ArgumentNullException: values が null です");
		return;
	}

	std::vector<Vector4> vectors(values->GetSize());
	for (asUINT i = 0; i < values->GetSize(); i++) {
		vectors[i] = *static_cast<const Vector4 *>(values->At(i));
	}
	shader->SetVectorArray(name, vectors);
}

// ComputeShader は Texture をポインタで覚える。RenderTexture を手放したら、Dispatch の前に SetTexture し直す
void SetComputeTexture(int kernelIndex, const std::string &name, ObjectHandle *texture, ObjectHandle *self) {
	auto *shader = Resolve<ComputeShader>(self);
	if (!shader) { return; }
	if (!texture) {
		ThrowException("ArgumentNullException: texture が null です");
		return;
	}
	if (auto *resolved = Resolve<Texture>(texture)) { shader->SetTexture(kernelIndex, name, resolved); }
}

void DispatchCompute(int kernelIndex, int threadGroupsX, int threadGroupsY, int threadGroupsZ, ObjectHandle *self) {
	if (auto *shader = Resolve<ComputeShader>(self)) { shader->Dispatch(kernelIndex, threadGroupsX, threadGroupsY, threadGroupsZ); }
}


// ---- LineRenderer ----

bool CheckPositionIndex(const LineRenderer &line, int index) {
	if (0 <= index && index < static_cast<int>(line.GetPositionCount())) { return true; }
	ThrowException(std::format("IndexOutOfRangeException: position の番号が範囲外です ({} / positionCount {})", index, line.GetPositionCount()));
	return false;
}

int GetPositionCount(ObjectHandle *self) {
	auto *line = Resolve<LineRenderer>(self);
	return line ? static_cast<int>(line->GetPositionCount()) : 0;
}

void SetPositionCount(int count, ObjectHandle *self) {
	if (count < 0) {
		ThrowException(std::format("ArgumentException: positionCount は 0 以上にしてください ({})", count));
		return;
	}
	if (auto *line = Resolve<LineRenderer>(self)) { line->SetPositionCount(static_cast<uint32_t>(count)); }
}

Vector3 GetLinePosition(int index, ObjectHandle *self) {
	auto *line = Resolve<LineRenderer>(self);
	if (!line || !CheckPositionIndex(*line, index)) { return Vector3::Zero; }
	return line->GetPosition(static_cast<uint32_t>(index));
}

void SetLinePosition(int index, const Vector3 &position, ObjectHandle *self) {
	auto *line = Resolve<LineRenderer>(self);
	if (line && CheckPositionIndex(*line, index)) { line->SetPosition(static_cast<uint32_t>(index), position); }
}

// Unity と同じく、positionCount は変えずに、前から入るだけ入れる
void SetLinePositions(CScriptArray *positions, ObjectHandle *self) {
	auto *line = Resolve<LineRenderer>(self);
	if (!line) { return; }
	if (!positions) {
		ThrowException("ArgumentNullException: positions が null です");
		return;
	}
	const auto count = std::min<asUINT>(positions->GetSize(), line->GetPositionCount());
	for (asUINT i = 0; i < count; i++) { line->SetPosition(i, *static_cast<const Vector3 *>(positions->At(i))); }
}

// Unity と同じく、渡した配列に入るだけ写し、写した数を返す
int GetLinePositions(CScriptArray *positions, ObjectHandle *self) {
	auto *line = Resolve<LineRenderer>(self);
	if (!line) { return 0; }
	if (!positions) {
		ThrowException("ArgumentNullException: positions が null です");
		return 0;
	}
	const auto count = std::min<asUINT>(positions->GetSize(), line->GetPositionCount());
	for (asUINT i = 0; i < count; i++) { *static_cast<Vector3 *>(positions->At(i)) = line->GetPosition(i); }
	return static_cast<int>(count);
}


// ---- SkeletalAnimator（Unity の Animation に近い。クリップを直接渡す） ----

void PlayClip(ObjectHandle *clip, ObjectHandle *self) {
	auto *animator = Resolve<SkeletalAnimator>(self);
	if (!animator) { return; }

	std::shared_ptr<SkeletalAnimationClip> shared;
	if (ResolveSharedAsset(clip, shared)) { animator->Play(std::move(shared)); }
}

void CrossFadeClip(ObjectHandle *clip, float fadeLength, ObjectHandle *self) {
	auto *animator = Resolve<SkeletalAnimator>(self);
	if (!animator) { return; }

	std::shared_ptr<SkeletalAnimationClip> shared;
	if (ResolveSharedAsset(clip, shared)) { animator->CrossFade(std::move(shared), fadeLength); }
}


// ---- ParticleSystem ----

void Emit(int count, ObjectHandle *self) {
	if (count < 0) { return; }
	if (auto *system = Resolve<ParticleSystem>(self)) { system->Emit(static_cast<uint32_t>(count)); }
}


// ---- ParticleSystem のモジュール（Unity と同じく、ParticleSystem を指す値。読み書きはその場で ParticleSystem に通す） ----
// スクリプトでは var main = ps.main; main.duration = 3; と書く。一時的な値にも書けるよう、set は const にする

// 既定の値を書かない（trivial にする）。書くと MSVC は 4 バイトでも返り値をメモリで渡すが、AngelScript はレジスタで受け取ろうとして食い違う
struct ScriptParticleModule {
	uint32_t ownerId;
};

ParticleSystem *ResolveOwner(const ScriptParticleModule *self) {
	auto *system = dynamic_cast<ParticleSystem *>(Object::FindObjectFromInstanceId(self->ownerId));
	if (!system) { ThrowException("MissingReferenceException: このモジュールの ParticleSystem は破棄されています"); }
	return system;
}

ParticleSystem::MainModule &MainOf(ParticleSystem &system) { return system.GetMain(); }
ParticleSystem::EmissionModule &EmissionOf(ParticleSystem &system) { return system.GetEmission(); }
ParticleSystem::ShapeModule &ShapeOf(ParticleSystem &system) { return system.GetShape(); }
ParticleSystem::ColorOverLifetimeModule &ColorOverLifetimeOf(ParticleSystem &system) { return system.GetColorOverLifetime(); }
ParticleSystem::SizeOverLifetimeModule &SizeOverLifetimeOf(ParticleSystem &system) { return system.GetSizeOverLifetime(); }

template <typename Module, Module &(*Access)(ParticleSystem &), typename V, V Module::*Field>
V GetModuleField(const ScriptParticleModule *self) {
	auto *system = ResolveOwner(self);
	return system ? Access(*system).*Field : V{};
}

template <typename Module, Module &(*Access)(ParticleSystem &), typename V, V Module::*Field>
void SetModuleField(const V &value, const ScriptParticleModule *self) {
	if (auto *system = ResolveOwner(self)) { Access(*system).*Field = value; }
}

// スクリプトの int・enum と C++ の uint32_t・enum をつなぐ
template <typename Module, Module &(*Access)(ParticleSystem &), typename V, V Module::*Field>
int GetModuleInt(const ScriptParticleModule *self) {
	auto *system = ResolveOwner(self);
	return system ? static_cast<int>(Access(*system).*Field) : 0;
}

template <typename Module, Module &(*Access)(ParticleSystem &), typename V, V Module::*Field>
void SetModuleInt(int value, const ScriptParticleModule *self) {
	if constexpr (std::is_enum_v<V>) {
		if (!CheckEnumValue<V>(value)) { return; }
	} else if (value < 0) {
		ThrowException(std::format("ArgumentException: 0 以上にしてください ({})", value));
		return;
	}
	if (auto *system = ResolveOwner(self)) { Access(*system).*Field = static_cast<V>(value); }
}

ScriptParticleModule GetModule(ObjectHandle *self) {
	auto *system = Resolve<ParticleSystem>(self);
	return {system ? system->GetInstanceId() : 0u};
}

void ConstructModule(void *memory) { new (memory) ScriptParticleModule{0}; }


// ---- MinMaxCurve・MinMaxGradient（C++ の FloatRange・ColorRange。Unity の定数の 2 つの間のランダム） ----

void ConstructCurve(void *memory) { new (memory) FloatRange(); }
void ConstructCurveConstant(float constant, void *memory) { new (memory) FloatRange(constant); }
void ConstructCurveRange(float min, float max, void *memory) { new (memory) FloatRange(min, max); }
float GetCurveConstant(const FloatRange *self) { return self->max; }
void SetCurveConstant(float value, FloatRange *self) { *self = FloatRange(value); }

void ConstructGradient(void *memory) { new (memory) ColorRange(); }
void ConstructGradientColor(const Color &color, void *memory) { new (memory) ColorRange(color); }
void ConstructGradientRange(const Color &min, const Color &max, void *memory) { new (memory) ColorRange(min, max); }
Color GetGradientColor(const ColorRange *self) { return self->max; }
void SetGradientColor(const Color &value, ColorRange *self) { *self = ColorRange(value); }


// モジュールの enabled（どのモジュールにもある）
template <typename Module, Module &(*Access)(ParticleSystem &)>
void RegisterModuleEnabled(asIScriptEngine *engine, const char *type) {
	Check(engine->RegisterObjectMethod(type, "bool get_enabled() const property", asFUNCTION((GetModuleField<Module, Access, bool, &Module::enabled>)), asCALL_CDECL_OBJLAST), type);
	Check(engine->RegisterObjectMethod(type, "void set_enabled(const bool &in value) const property", asFUNCTION((SetModuleField<Module, Access, bool, &Module::enabled>)), asCALL_CDECL_OBJLAST), type);
}

} // namespace



void ou::ScriptBindings::RegisterRenderMembers(asIScriptEngine *engine) {
	// ---- enum ----
	RegisterEnum<LightType>(engine, "LightType");
	RegisterEnum<LightShadows>(engine, "LightShadows");
	RegisterEnum<LightShadowResolution>(engine, "LightShadowResolution");
	RegisterEnum<WrapMode>(engine, "WrapMode");
	RegisterEnum<ParticleSystem::SimulationSpace>(engine, "ParticleSystemSimulationSpace");
	RegisterEnum<ParticleSystem::ShapeType>(engine, "ParticleSystemShapeType");
	RegisterEnum<ParticleSystemRenderer::SortMode>(engine, "ParticleSystemSortMode");

	// ---- Camera ----
	{
		const auto c = RegisterComponentType(engine, "Camera");
		RegisterBehaviorMembers(c);
		// 度（Unity と同じ）
		c.Method("float get_fieldOfView() const property", asFUNCTION((Get<Camera, float, &Camera::GetFieldOfView>)));
		c.Method("void set_fieldOfView(float value) property", asFUNCTION((Set<Camera, float, &Camera::SetFieldOfView>)));
		c.Method("float get_aspect() const property", asFUNCTION((Get<Camera, float, &Camera::GetAspect>)));
		c.Method("void set_aspect(float value) property", asFUNCTION((Set<Camera, float, &Camera::SetAspect>)));
		c.Method("void ResetAspect()", asFUNCTION((Call<Camera, &Camera::ResetAspect>)));
		c.Method("float get_nearClipPlane() const property", asFUNCTION((Get<Camera, float, &Camera::GetNearClipPlane>)));
		c.Method("void set_nearClipPlane(float value) property", asFUNCTION((Set<Camera, float, &Camera::SetNearClipPlane>)));
		c.Method("float get_farClipPlane() const property", asFUNCTION((Get<Camera, float, &Camera::GetFarClipPlane>)));
		c.Method("void set_farClipPlane(float value) property", asFUNCTION((Set<Camera, float, &Camera::SetFarClipPlane>)));

		engine->SetDefaultNamespace("Camera");
		Check(engine->RegisterGlobalFunction("Camera@ get_main() property", asFUNCTION(GetMainCamera), asCALL_CDECL), "Camera::main");
		engine->SetDefaultNamespace("");
	}

	// ---- Light ----
	{
		const auto l = RegisterComponentType(engine, "Light");
		RegisterBehaviorMembers(l);
		l.Method("LightType get_type() const property", asFUNCTION((GetEnum<Light, LightType, &Light::GetType>)));
		l.Method("void set_type(LightType value) property", asFUNCTION((SetEnum<Light, LightType, &Light::SetType>)));
		l.Method("Color get_color() const property", asFUNCTION((Get<Light, const Color &, &Light::GetColor>)));
		l.Method("void set_color(const Color &in value) property", asFUNCTION((Set<Light, const Color &, &Light::SetColor>)));
		l.Method("float get_intensity() const property", asFUNCTION((Get<Light, float, &Light::GetIntensity>)));
		l.Method("void set_intensity(float value) property", asFUNCTION((Set<Light, float, &Light::SetIntensity>)));
		l.Method("float get_range() const property", asFUNCTION((Get<Light, float, &Light::GetRange>)));
		l.Method("void set_range(float value) property", asFUNCTION((Set<Light, float, &Light::SetRange>)));
		l.Method("float get_spotAngle() const property", asFUNCTION((Get<Light, float, &Light::GetSpotAngle>)));
		l.Method("void set_spotAngle(float value) property", asFUNCTION((Set<Light, float, &Light::SetSpotAngle>)));
		l.Method("float get_innerSpotAngle() const property", asFUNCTION((Get<Light, float, &Light::GetInnerSpotAngle>)));
		l.Method("void set_innerSpotAngle(float value) property", asFUNCTION((Set<Light, float, &Light::SetInnerSpotAngle>)));
		l.Method("LightShadows get_shadows() const property", asFUNCTION((GetEnum<Light, LightShadows, &Light::GetShadows>)));
		l.Method("void set_shadows(LightShadows value) property", asFUNCTION((SetEnum<Light, LightShadows, &Light::SetShadows>)));
		l.Method("float get_shadowStrength() const property", asFUNCTION((Get<Light, float, &Light::GetShadowStrength>)));
		l.Method("void set_shadowStrength(float value) property", asFUNCTION((Set<Light, float, &Light::SetShadowStrength>)));
		l.Method("float get_shadowBias() const property", asFUNCTION((Get<Light, float, &Light::GetShadowBias>)));
		l.Method("void set_shadowBias(float value) property", asFUNCTION((Set<Light, float, &Light::SetShadowBias>)));
		l.Method("float get_shadowNormalBias() const property", asFUNCTION((Get<Light, float, &Light::GetShadowNormalBias>)));
		l.Method("void set_shadowNormalBias(float value) property", asFUNCTION((Set<Light, float, &Light::SetShadowNormalBias>)));
		l.Method("LightShadowResolution get_shadowResolution() const property", asFUNCTION((GetEnum<Light, LightShadowResolution, &Light::GetShadowResolution>)));
		l.Method("void set_shadowResolution(LightShadowResolution value) property", asFUNCTION((SetEnum<Light, LightShadowResolution, &Light::SetShadowResolution>)));
		l.Method("float get_shadowNearPlane() const property", asFUNCTION((Get<Light, float, &Light::GetShadowNearPlane>)));
		l.Method("void set_shadowNearPlane(float value) property", asFUNCTION((Set<Light, float, &Light::SetShadowNearPlane>)));
	}

	// ---- Mesh・Shader・SkeletalAnimationClip・VolumeProfile（アセット） ----
	{
		const auto m = RegisterAssetType(engine, "Mesh");
		m.Method("Bounds get_bounds() const property", asFUNCTION((Get<Mesh, const Bounds &, &Mesh::GetBounds>)));
		m.Method("int get_vertexCount() const property", asFUNCTION(GetVertexCount));
		m.Method("int get_subMeshCount() const property", asFUNCTION(GetSubMeshCount));
	}
	{
		const auto s = RegisterAssetType(engine, "Shader");
		s.Method("int get_renderQueue() const property", asFUNCTION((Get<Shader, int, &Shader::GetRenderQueue>)));
		s.Method("int get_passCount() const property", asFUNCTION(GetPassCount));
		s.Method("int FindPropertyIndex(const string &in propertyName) const", asFUNCTION(FindPropertyIndex));
	}
	{
		const auto c = RegisterAssetType(engine, "ComputeShader");
		c.Method("int FindKernel(const string &in name) const", asFUNCTION(FindKernel));
		c.Method("bool HasKernel(const string &in name) const", asFUNCTION(HasKernel));
		c.Method("void GetKernelThreadGroupSizes(int kernelIndex, uint &out x, uint &out y, uint &out z) const", asFUNCTION(GetKernelThreadGroupSizes));
		c.Method("void SetFloat(const string &in name, float value)", asFUNCTION((SetComputeValue<float, &ComputeShader::SetFloat>)));
		c.Method("void SetInt(const string &in name, int value)", asFUNCTION((SetComputeValue<int, &ComputeShader::SetInt>)));
		c.Method("void SetBool(const string &in name, bool value)", asFUNCTION((SetComputeValue<bool, &ComputeShader::SetBool>)));
		c.Method("void SetVector(const string &in name, const Vector4 &in value)", asFUNCTION((SetComputeValue<const Vector4 &, &ComputeShader::SetVector>)));
		c.Method("void SetVectorArray(const string &in name, array<Vector4>@+ values)", asFUNCTION(SetComputeVectorArray));
		c.Method("void SetMatrix(const string &in name, const Matrix4x4 &in value)", asFUNCTION((SetComputeValue<const Matrix4x4 &, &ComputeShader::SetMatrix>)));
		c.Method("void SetTexture(int kernelIndex, const string &in name, Texture@+ texture)", asFUNCTION(SetComputeTexture));
		c.Method("void Dispatch(int kernelIndex, int threadGroupsX, int threadGroupsY, int threadGroupsZ)", asFUNCTION(DispatchCompute));
	}
	{
		const auto a = RegisterAssetType(engine, "SkeletalAnimationClip");
		a.Method("float get_length() const property", asFUNCTION((Get<SkeletalAnimationClip, float, &SkeletalAnimationClip::GetLength>)));
		a.Method("WrapMode get_wrapMode() const property", asFUNCTION((GetEnum<SkeletalAnimationClip, WrapMode, &SkeletalAnimationClip::GetWrapMode>)));
		a.Method("void set_wrapMode(WrapMode value) property", asFUNCTION((SetEnum<SkeletalAnimationClip, WrapMode, &SkeletalAnimationClip::SetWrapMode>)));
	}
	RegisterAssetType(engine, "VolumeProfile");

	// ---- SkinnedMeshRenderer ----
	{
		const auto r = RegisterComponentType(engine, "SkinnedMeshRenderer");
		RegisterRendererMembers(r);
		r.Method("Mesh@ get_sharedMesh() const property", asFUNCTION((GetAssetHandle<SkinnedMeshRenderer, Mesh, &SkinnedMeshRenderer::GetMesh>)));
		r.Method("void set_sharedMesh(Mesh@+ value) property", asFUNCTION((SetAssetHandle<SkinnedMeshRenderer, Mesh, &SkinnedMeshRenderer::SetMesh>)));
	}

	// ---- LineRenderer ----
	{
		const auto r = RegisterComponentType(engine, "LineRenderer");
		RegisterRendererMembers(r);
		r.Method("int get_positionCount() const property", asFUNCTION(GetPositionCount));
		r.Method("void set_positionCount(int value) property", asFUNCTION(SetPositionCount));
		r.Method("Vector3 GetPosition(int index) const", asFUNCTION(GetLinePosition));
		r.Method("void SetPosition(int index, const Vector3 &in position)", asFUNCTION(SetLinePosition));
		r.Method("void SetPositions(array<Vector3>@+ positions)", asFUNCTION(SetLinePositions));
		r.Method("int GetPositions(array<Vector3>@+ positions) const", asFUNCTION(GetLinePositions));
		r.Method("float get_startWidth() const property", asFUNCTION((Get<LineRenderer, float, &LineRenderer::GetStartWidth>)));
		r.Method("void set_startWidth(float value) property", asFUNCTION((Set<LineRenderer, float, &LineRenderer::SetStartWidth>)));
		r.Method("float get_endWidth() const property", asFUNCTION((Get<LineRenderer, float, &LineRenderer::GetEndWidth>)));
		r.Method("void set_endWidth(float value) property", asFUNCTION((Set<LineRenderer, float, &LineRenderer::SetEndWidth>)));
		r.Method("float get_widthMultiplier() const property", asFUNCTION((Get<LineRenderer, float, &LineRenderer::GetWidthMultiplier>)));
		r.Method("void set_widthMultiplier(float value) property", asFUNCTION((Set<LineRenderer, float, &LineRenderer::SetWidthMultiplier>)));
		r.Method("Color get_startColor() const property", asFUNCTION((Get<LineRenderer, const Color &, &LineRenderer::GetStartColor>)));
		r.Method("void set_startColor(const Color &in value) property", asFUNCTION((Set<LineRenderer, const Color &, &LineRenderer::SetStartColor>)));
		r.Method("Color get_endColor() const property", asFUNCTION((Get<LineRenderer, const Color &, &LineRenderer::GetEndColor>)));
		r.Method("void set_endColor(const Color &in value) property", asFUNCTION((Set<LineRenderer, const Color &, &LineRenderer::SetEndColor>)));
		r.Method("bool get_useWorldSpace() const property", asFUNCTION((Get<LineRenderer, bool, &LineRenderer::GetUseWorldSpace>)));
		r.Method("void set_useWorldSpace(bool value) property", asFUNCTION((Set<LineRenderer, bool, &LineRenderer::SetUseWorldSpace>)));
		r.Method("bool get_loop() const property", asFUNCTION((Get<LineRenderer, bool, &LineRenderer::GetLoop>)));
		r.Method("void set_loop(bool value) property", asFUNCTION((Set<LineRenderer, bool, &LineRenderer::SetLoop>)));
	}

	// ---- ParticleSystemRenderer ----
	{
		const auto r = RegisterComponentType(engine, "ParticleSystemRenderer");
		RegisterRendererMembers(r);
		r.Method("ParticleSystemSortMode get_sortMode() const property", asFUNCTION((GetEnum<ParticleSystemRenderer, ParticleSystemRenderer::SortMode, &ParticleSystemRenderer::GetSortMode>)));
		r.Method("void set_sortMode(ParticleSystemSortMode value) property", asFUNCTION((SetEnum<ParticleSystemRenderer, ParticleSystemRenderer::SortMode, &ParticleSystemRenderer::SetSortMode>)));
	}

	// ---- SkeletalAnimator ----
	{
		const auto a = RegisterComponentType(engine, "SkeletalAnimator");
		RegisterBehaviorMembers(a);
		a.Method("void Play(SkeletalAnimationClip@+ clip)", asFUNCTION(PlayClip));
		a.Method("void CrossFade(SkeletalAnimationClip@+ clip, float fadeLength = 0.3f)", asFUNCTION(CrossFadeClip));
		a.Method("void Stop()", asFUNCTION((Call<SkeletalAnimator, &SkeletalAnimator::Stop>)));
		a.Method("bool get_isPlaying() const property", asFUNCTION((Get<SkeletalAnimator, bool, &SkeletalAnimator::IsPlaying>)));
		a.Method("SkeletalAnimationClip@ get_clip() const property", asFUNCTION((GetAssetHandle<SkeletalAnimator, SkeletalAnimationClip, &SkeletalAnimator::GetClip>)));
		a.Method("void set_clip(SkeletalAnimationClip@+ value) property", asFUNCTION((SetAssetHandle<SkeletalAnimator, SkeletalAnimationClip, &SkeletalAnimator::SetClip>)));
		a.Method("float get_time() const property", asFUNCTION((Get<SkeletalAnimator, float, &SkeletalAnimator::GetTime>)));
		a.Method("void set_time(float value) property", asFUNCTION((Set<SkeletalAnimator, float, &SkeletalAnimator::SetTime>)));
		a.Method("float get_speed() const property", asFUNCTION((Get<SkeletalAnimator, float, &SkeletalAnimator::GetSpeed>)));
		a.Method("void set_speed(float value) property", asFUNCTION((Set<SkeletalAnimator, float, &SkeletalAnimator::SetSpeed>)));
		a.Method("WrapMode get_wrapMode() const property", asFUNCTION((GetEnum<SkeletalAnimator, WrapMode, &SkeletalAnimator::GetWrapMode>)));
		a.Method("void set_wrapMode(WrapMode value) property", asFUNCTION((SetEnum<SkeletalAnimator, WrapMode, &SkeletalAnimator::SetWrapMode>)));
		a.Method("bool get_playAutomatically() const property", asFUNCTION((Get<SkeletalAnimator, bool, &SkeletalAnimator::IsPlayAutomatically>)));
		a.Method("void set_playAutomatically(bool value) property", asFUNCTION((Set<SkeletalAnimator, bool, &SkeletalAnimator::SetPlayAutomatically>)));
	}

	// ---- ParticleSystem のモジュールの型（Unity と同じく ParticleSystem::MainModule のように書く） ----
	engine->SetDefaultNamespace("ParticleSystem");
	for (const auto *type : {"MainModule", "EmissionModule", "ShapeModule", "ColorOverLifetimeModule", "SizeOverLifetimeModule"}) {
		Check(engine->RegisterObjectType(type, sizeof(ScriptParticleModule), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<ScriptParticleModule>()), type);
		Check(engine->RegisterObjectBehaviour(type, asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructModule), asCALL_CDECL_OBJLAST), type);
	}
	Check(engine->RegisterObjectType("MinMaxCurve", sizeof(FloatRange), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_ALLFLOATS | asGetTypeTraits<FloatRange>()), "MinMaxCurve");
	Check(engine->RegisterObjectType("MinMaxGradient", sizeof(ColorRange), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_ALLFLOATS | asGetTypeTraits<ColorRange>()), "MinMaxGradient");

	// 型と同じく ParticleSystem の名前空間の中で登録する（型の名前は名前空間を付けずに書く）
	const auto method = [&](const char *type, const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterObjectMethod(type, declaration, function, asCALL_CDECL_OBJLAST), declaration);
	};
	const auto behaviour = [&](const char *type, const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterObjectBehaviour(type, asBEHAVE_CONSTRUCT, declaration, function, asCALL_CDECL_OBJLAST), declaration);
	};

	// ---- MinMaxCurve（float から暗黙に変えられる。main.startSize = 2.0f と書ける） ----
	{
		const auto *type = "MinMaxCurve";
		behaviour(type, "void f()", asFUNCTION(ConstructCurve));
		behaviour(type, "void f(float constant)", asFUNCTION(ConstructCurveConstant));
		behaviour(type, "void f(float min, float max)", asFUNCTION(ConstructCurveRange));
		Check(engine->RegisterObjectProperty(type, "float constantMin", asOFFSET(FloatRange, min)), "MinMaxCurve::constantMin");
		Check(engine->RegisterObjectProperty(type, "float constantMax", asOFFSET(FloatRange, max)), "MinMaxCurve::constantMax");
		method(type, "float get_constant() const property", asFUNCTION(GetCurveConstant));
		method(type, "void set_constant(float value) property", asFUNCTION(SetCurveConstant));
	}

	// ---- MinMaxGradient（Color から暗黙に変えられる） ----
	{
		const auto *type = "MinMaxGradient";
		behaviour(type, "void f()", asFUNCTION(ConstructGradient));
		behaviour(type, "void f(const Color &in color)", asFUNCTION(ConstructGradientColor));
		behaviour(type, "void f(const Color &in min, const Color &in max)", asFUNCTION(ConstructGradientRange));
		Check(engine->RegisterObjectProperty(type, "Color colorMin", asOFFSET(ColorRange, min)), "MinMaxGradient::colorMin");
		Check(engine->RegisterObjectProperty(type, "Color colorMax", asOFFSET(ColorRange, max)), "MinMaxGradient::colorMax");
		method(type, "Color get_color() const property", asFUNCTION(GetGradientColor));
		method(type, "void set_color(const Color &in value) property", asFUNCTION(SetGradientColor));
	}

	// ---- MainModule ----
	{
		using M = ParticleSystem::MainModule;
		const auto *type = "MainModule";
		method(type, "float get_duration() const property", asFUNCTION((GetModuleField<M, MainOf, float, &M::duration>)));
		method(type, "void set_duration(const float &in value) const property", asFUNCTION((SetModuleField<M, MainOf, float, &M::duration>)));
		method(type, "bool get_loop() const property", asFUNCTION((GetModuleField<M, MainOf, bool, &M::looping>)));
		method(type, "void set_loop(const bool &in value) const property", asFUNCTION((SetModuleField<M, MainOf, bool, &M::looping>)));
		method(type, "bool get_playOnAwake() const property", asFUNCTION((GetModuleField<M, MainOf, bool, &M::playOnAwake>)));
		method(type, "void set_playOnAwake(const bool &in value) const property", asFUNCTION((SetModuleField<M, MainOf, bool, &M::playOnAwake>)));
		method(type, "MinMaxCurve get_startLifetime() const property", asFUNCTION((GetModuleField<M, MainOf, FloatRange, &M::startLifetime>)));
		method(type, "void set_startLifetime(const MinMaxCurve &in value) const property", asFUNCTION((SetModuleField<M, MainOf, FloatRange, &M::startLifetime>)));
		method(type, "MinMaxCurve get_startSpeed() const property", asFUNCTION((GetModuleField<M, MainOf, FloatRange, &M::startSpeed>)));
		method(type, "void set_startSpeed(const MinMaxCurve &in value) const property", asFUNCTION((SetModuleField<M, MainOf, FloatRange, &M::startSpeed>)));
		method(type, "MinMaxCurve get_startSize() const property", asFUNCTION((GetModuleField<M, MainOf, FloatRange, &M::startSize>)));
		method(type, "void set_startSize(const MinMaxCurve &in value) const property", asFUNCTION((SetModuleField<M, MainOf, FloatRange, &M::startSize>)));
		method(type, "MinMaxGradient get_startColor() const property", asFUNCTION((GetModuleField<M, MainOf, ColorRange, &M::startColor>)));
		method(type, "void set_startColor(const MinMaxGradient &in value) const property", asFUNCTION((SetModuleField<M, MainOf, ColorRange, &M::startColor>)));
		// C++ は定数だけなので、Unity の MinMaxCurve ではなく float で出す（main.gravityModifier = 0.5f は同じく書ける）
		method(type, "float get_gravityModifier() const property", asFUNCTION((GetModuleField<M, MainOf, float, &M::gravityModifier>)));
		method(type, "void set_gravityModifier(const float &in value) const property", asFUNCTION((SetModuleField<M, MainOf, float, &M::gravityModifier>)));
		method(type, "int get_maxParticles() const property", asFUNCTION((GetModuleInt<M, MainOf, uint32_t, &M::maxParticles>)));
		method(type, "void set_maxParticles(int value) const property", asFUNCTION((SetModuleInt<M, MainOf, uint32_t, &M::maxParticles>)));
		method(type, "ParticleSystemSimulationSpace get_simulationSpace() const property", asFUNCTION((GetModuleInt<M, MainOf, ParticleSystem::SimulationSpace, &M::simulationSpace>)));
		method(type, "void set_simulationSpace(ParticleSystemSimulationSpace value) const property", asFUNCTION((SetModuleInt<M, MainOf, ParticleSystem::SimulationSpace, &M::simulationSpace>)));
	}

	// ---- EmissionModule ----
	{
		using M = ParticleSystem::EmissionModule;
		const auto *type = "EmissionModule";
		RegisterModuleEnabled<M, EmissionOf>(engine, type);
		// C++ は定数だけなので float で出す（emission.rateOverTime = 20.0f は同じく書ける）
		method(type, "float get_rateOverTime() const property", asFUNCTION((GetModuleField<M, EmissionOf, float, &M::rateOverTime>)));
		method(type, "void set_rateOverTime(const float &in value) const property", asFUNCTION((SetModuleField<M, EmissionOf, float, &M::rateOverTime>)));
	}

	// ---- ShapeModule ----
	{
		using M = ParticleSystem::ShapeModule;
		const auto *type = "ShapeModule";
		RegisterModuleEnabled<M, ShapeOf>(engine, type);
		method(type, "ParticleSystemShapeType get_shapeType() const property", asFUNCTION((GetModuleInt<M, ShapeOf, ParticleSystem::ShapeType, &M::shapeType>)));
		method(type, "void set_shapeType(ParticleSystemShapeType value) const property", asFUNCTION((SetModuleInt<M, ShapeOf, ParticleSystem::ShapeType, &M::shapeType>)));
		method(type, "float get_angle() const property", asFUNCTION((GetModuleField<M, ShapeOf, float, &M::angle>)));
		method(type, "void set_angle(const float &in value) const property", asFUNCTION((SetModuleField<M, ShapeOf, float, &M::angle>)));
		method(type, "float get_radius() const property", asFUNCTION((GetModuleField<M, ShapeOf, float, &M::radius>)));
		method(type, "void set_radius(const float &in value) const property", asFUNCTION((SetModuleField<M, ShapeOf, float, &M::radius>)));
		method(type, "float get_radiusThickness() const property", asFUNCTION((GetModuleField<M, ShapeOf, float, &M::radiusThickness>)));
		method(type, "void set_radiusThickness(const float &in value) const property", asFUNCTION((SetModuleField<M, ShapeOf, float, &M::radiusThickness>)));
	}

	// ---- ColorOverLifetimeModule・SizeOverLifetimeModule（C++ は始まりと終わりの 2 点なので、Unity の Gradient・AnimationCurve はまだ出さない） ----
	RegisterModuleEnabled<ParticleSystem::ColorOverLifetimeModule, ColorOverLifetimeOf>(engine, "ColorOverLifetimeModule");
	RegisterModuleEnabled<ParticleSystem::SizeOverLifetimeModule, SizeOverLifetimeOf>(engine, "SizeOverLifetimeModule");
	engine->SetDefaultNamespace("");

	// ---- ParticleSystem ----
	{
		const auto p = RegisterComponentType(engine, "ParticleSystem");
		p.Method("void Play()", asFUNCTION((Call<ParticleSystem, &ParticleSystem::Play>)));
		p.Method("void Stop()", asFUNCTION((Call<ParticleSystem, &ParticleSystem::Stop>)));
		p.Method("void Clear()", asFUNCTION((Call<ParticleSystem, &ParticleSystem::Clear>)));
		p.Method("void Emit(int count)", asFUNCTION(Emit));
		p.Method("void Simulate(float t)", asFUNCTION((Set<ParticleSystem, float, &ParticleSystem::Simulate>)));
		p.Method("bool get_isPlaying() const property", asFUNCTION((Get<ParticleSystem, bool, &ParticleSystem::IsPlaying>)));
		p.Method("float get_time() const property", asFUNCTION((Get<ParticleSystem, float, &ParticleSystem::GetTime>)));
		p.Method("ParticleSystem::MainModule get_main() const property", asFUNCTION(GetModule));
		p.Method("ParticleSystem::EmissionModule get_emission() const property", asFUNCTION(GetModule));
		p.Method("ParticleSystem::ShapeModule get_shape() const property", asFUNCTION(GetModule));
		p.Method("ParticleSystem::ColorOverLifetimeModule get_colorOverLifetime() const property", asFUNCTION(GetModule));
		p.Method("ParticleSystem::SizeOverLifetimeModule get_sizeOverLifetime() const property", asFUNCTION(GetModule));
	}
}
