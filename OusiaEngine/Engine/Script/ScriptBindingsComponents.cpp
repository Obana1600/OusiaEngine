#include "Asset/AssetDatabase.h"
#include "Audio/AudioClip.h"
#include "Audio/AudioListener.h"
#include "Audio/AudioSource.h"
#include "Math/Color.h"
#include "Math/Rect.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/RenderTexture.h"
#include "Render/Asset/Shader.h"
#include "Render/Asset/Texture.h"
#include "Render/Renderer.h"
#include "Script/ScriptBindingsInternal.h"
#include "Script/ScriptEngine.h"
#include "ScriptBindings.h"
#include "UI/Asset/FontAsset.h"
#include "UI/Asset/Sprite.h"
#include "UI/Button.h"
#include "UI/CanvasRenderer.h"
#include "UI/ColorBlock.h"
#include "UI/Graphic.h"
#include "UI/Image.h"
#include "UI/Selectable.h"
#include "UI/Text.h"

#include <cstdint>
#include <format>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include <angelscript.h>

using namespace ou;
using namespace ou::ScriptBindingsInternal;


namespace {

// ---- アセット（Object の name・GetInstanceID） ----

std::string GetAssetName(ObjectHandle *self) {
	auto *object = Resolve<Object>(self);
	return object ? object->GetName() : std::string();
}

void SetAssetName(const std::string &name, ObjectHandle *self) {
	if (auto *object = Resolve<Object>(self)) { object->SetName(name); }
}

int GetAssetInstanceID(ObjectHandle *self) {
	return static_cast<int>(self->instanceId);
}


// ---- AudioClip ----

float GetClipLength(ObjectHandle *self) {
	auto *clip = Resolve<AudioClip>(self);
	return clip ? clip->GetLength() : 0.0f;
}

int GetClipFrequency(ObjectHandle *self) {
	auto *clip = Resolve<AudioClip>(self);
	return clip ? static_cast<int>(clip->GetFrequency()) : 0;
}

int GetClipChannels(ObjectHandle *self) {
	auto *clip = Resolve<AudioClip>(self);
	return clip ? static_cast<int>(clip->GetChannels()) : 0;
}


// ---- Texture2D ----

int GetTextureWidth(ObjectHandle *self) {
	auto *texture = Resolve<Texture>(self);
	return texture ? static_cast<int>(texture->GetWidth()) : 0;
}

int GetTextureHeight(ObjectHandle *self) {
	auto *texture = Resolve<Texture>(self);
	return texture ? static_cast<int>(texture->GetHeight()) : 0;
}

bool IsTextureReadable(ObjectHandle *self) {
	auto *texture = Resolve<Texture>(self);
	return texture && texture->IsReadable();
}


// ---- RenderTexture ----

// Unity の new RenderTexture(width, height, depth, format)。アセットではないので保存されない。GPU の画像は Create か、初めて使うときに作る
ObjectHandle *CreateRenderTexture(int width, int height, int depth, int format) {
	if (width <= 0 || height <= 0) {
		ThrowException(std::format("ArgumentException: RenderTexture の大きさは 1 以上にしてください ({} x {})", width, height));
		return nullptr;
	}
	if (!CheckEnumValue<RenderTextureFormat>(format)) { return nullptr; }
	return MakeOwningHandle(std::make_shared<RenderTexture>(width, height, depth, static_cast<RenderTextureFormat>(format)));
}

bool CreateTextureResource(ObjectHandle *self) {
	auto *texture = Resolve<RenderTexture>(self);
	return texture && texture->Create();
}


// ---- AudioSource ----

template <void (AudioSource::*Function)()>
void CallAudioSource(ObjectHandle *self) {
	if (auto *source = Resolve<AudioSource>(self)) { (source->*Function)(); }
}

ObjectHandle *GetClip(ObjectHandle *self) {
	auto *source = Resolve<AudioSource>(self);
	return source ? MakeHandle(source->GetClip().get()) : nullptr;
}

void SetClip(ObjectHandle *clip, ObjectHandle *self) {
	auto *source = Resolve<AudioSource>(self);
	if (!source) { return; }

	std::shared_ptr<AudioClip> shared;
	if (ResolveSharedAsset(clip, shared)) { source->SetClip(std::move(shared)); }
}

// Unity と同じく、clip を替えずに重ねて鳴らす
void PlayOneShot(ObjectHandle *clip, float volumeScale, ObjectHandle *self) {
	auto *source = Resolve<AudioSource>(self);
	if (!source || !clip) { return; }

	std::shared_ptr<AudioClip> shared;
	if (ResolveSharedAsset(clip, shared) && shared) { source->PlayOneShot(shared, volumeScale); }
}


// ---- Renderer ----

// Unity の renderer.material: 初めて触ったときに複製し、この Renderer だけの Material にする
ObjectHandle *GetMaterial(ObjectHandle *self) {
	auto *renderer = Resolve<Renderer>(self);
	return renderer ? MakeHandle(renderer->GetMaterial().get()) : nullptr;
}

ObjectHandle *GetSharedMaterial(ObjectHandle *self) {
	auto *renderer = Resolve<Renderer>(self);
	return renderer ? MakeHandle(renderer->GetSharedMaterial().get()) : nullptr;
}

// 入れられるのはアセットの Material だけ（ほかの Renderer が複製した material は、まだ入れられない）
void SetSharedMaterial(ObjectHandle *material, ObjectHandle *self) {
	auto *renderer = Resolve<Renderer>(self);
	if (!renderer) { return; }

	std::shared_ptr<Material> shared;
	if (ResolveSharedAsset(material, shared)) { renderer->SetSharedMaterial(std::move(shared)); }
}


// ---- Material ----

Color GetMaterialColor(ObjectHandle *self) {
	auto *material = Resolve<Material>(self);
	return material ? material->GetColor() : Color::White;
}

void SetMaterialColor(const Color &color, ObjectHandle *self) {
	if (auto *material = Resolve<Material>(self)) { material->SetColor(color); }
}

Color GetNamedColor(const std::string &name, ObjectHandle *self) {
	auto *material = Resolve<Material>(self);
	return material ? material->GetColor(name) : Color::Clear;
}

void SetNamedColor(const std::string &name, const Color &color, ObjectHandle *self) {
	if (auto *material = Resolve<Material>(self)) { material->SetColor(name, color); }
}

float GetFloat(const std::string &name, ObjectHandle *self) {
	auto *material = Resolve<Material>(self);
	return material ? material->GetFloat(name) : 0.0f;
}

void SetFloat(const std::string &name, float value, ObjectHandle *self) {
	if (auto *material = Resolve<Material>(self)) { material->SetFloat(name, value); }
}

bool HasProperty(const std::string &name, ObjectHandle *self) {
	auto *material = Resolve<Material>(self);
	return material && material->HasProperty(name);
}

Vector4 GetVector(const std::string &name, ObjectHandle *self) {
	auto *material = Resolve<Material>(self);
	return material ? material->GetVector(name) : Vector4::Zero;
}

void SetVector(const std::string &name, const Vector4 &value, ObjectHandle *self) {
	if (auto *material = Resolve<Material>(self)) { material->SetVector(name, value); }
}

ObjectHandle *GetNamedTexture(const std::string &name, ObjectHandle *self) {
	auto *material = Resolve<Material>(self);
	return material ? MakeHandle(material->GetTexture(name).get()) : nullptr;
}

// 入れられるのは、アセットかスクリプトで作ったもの（new RenderTexture など）だけ
void SetNamedTexture(const std::string &name, ObjectHandle *texture, ObjectHandle *self) {
	auto *material = Resolve<Material>(self);
	if (!material) { return; }

	std::shared_ptr<Texture> shared;
	if (ResolveSharedAsset(texture, shared)) { material->SetTexture(name, std::move(shared)); }
}

// Unity の new Material(shader)・new Material(source)。アセットではないので保存されない
ObjectHandle *CreateMaterialFromShader(ObjectHandle *shader) {
	std::shared_ptr<Shader> shared;
	if (!ResolveSharedAsset(shader, shared)) { return nullptr; }
	return MakeOwningHandle(Material::Create(std::move(shared)));
}

ObjectHandle *CreateMaterialFromSource(ObjectHandle *source) {
	if (!source) {
		ThrowException("ArgumentNullException: source が null です");
		return nullptr;
	}
	auto *material = Resolve<Material>(source);
	return material ? MakeOwningHandle(Material::Create(*material)) : nullptr;
}


// ---- Renderer.materials・sharedMaterials（Unity の Material[]） ----

CScriptArray *MakeMaterialArray(const std::vector<std::shared_ptr<Material>> &shared) {
	std::vector<Material *> materials;
	for (const auto &material : shared) {
		materials.push_back(material.get());
	}
	return MakeHandleArray(materials, "array<Material@>");
}

// Unity の renderer.materials: 初めて触ったときに複製し、この Renderer だけの Material にする
CScriptArray *GetMaterials(ObjectHandle *self) {
	auto *renderer = Resolve<Renderer>(self);
	return MakeMaterialArray(renderer ? renderer->GetMaterials() : std::vector<std::shared_ptr<Material>>{});
}

CScriptArray *GetSharedMaterials(ObjectHandle *self) {
	auto *renderer = Resolve<Renderer>(self);
	return MakeMaterialArray(renderer ? renderer->GetSharedMaterials() : std::vector<std::shared_ptr<Material>>{});
}

// 入れられるのは、アセットかスクリプトで作った Material だけ（material の set と同じ）
void SetSharedMaterials(CScriptArray *materials, ObjectHandle *self) {
	auto *renderer = Resolve<Renderer>(self);
	if (!renderer) { return; }
	if (!materials) {
		ThrowException("ArgumentNullException: materials が null です");
		return;
	}

	std::vector<std::shared_ptr<Material>> shared(materials->GetSize());
	for (asUINT i = 0; i < materials->GetSize(); i++) {
		auto *handle = *static_cast<ObjectHandle **>(materials->At(i));
		if (!ResolveSharedAsset(handle, shared[i])) { return; }
	}
	renderer->SetSharedMaterials(std::move(shared));
}


// ---- Graphic・Image・Text ----

ObjectHandle *GetGraphicMaterial(ObjectHandle *self) {
	auto *graphic = Resolve<Graphic>(self);
	if (!graphic) { return nullptr; }
	// Unity と同じく、入れていなければ既定の Material を返す
	const auto &material = graphic->GetMaterial();
	return MakeHandle(material ? material.get() : graphic->GetDefaultMaterial().get());
}


// ---- Selectable ----

void SetTargetGraphic(ObjectHandle *graphic, ObjectHandle *self) {
	auto *selectable = Resolve<Selectable>(self);
	if (!selectable) { return; }

	Graphic *target = nullptr;
	if (graphic) {
		target = Resolve<Graphic>(graphic);
		if (!target) { return; }
	}
	selectable->SetTargetGraphic(target);
}

ObjectHandle *FindSelectable(const Vector3 &direction, ObjectHandle *self) {
	auto *selectable = Resolve<Selectable>(self);
	return selectable ? MakeHandle(selectable->FindSelectable(direction)) : nullptr;
}

CScriptArray *GetAllSelectables() {
	return MakeHandleArray(Selectable::GetAllSelectables(), "array<Selectable@>");
}


// ---- CanvasRenderer ----

int GetMaterialCount(ObjectHandle *self) {
	auto *renderer = Resolve<CanvasRenderer>(self);
	return renderer ? static_cast<int>(renderer->GetMaterialCount()) : 0;
}

void SetMaterialCount(int count, ObjectHandle *self) {
	if (count < 0) {
		ThrowException(std::format("ArgumentException: materialCount は 0 以上にしてください ({})", count));
		return;
	}
	if (auto *renderer = Resolve<CanvasRenderer>(self)) { renderer->SetMaterialCount(static_cast<uint32_t>(count)); }
}

bool CheckMaterialIndex(const CanvasRenderer &renderer, int index) {
	if (0 <= index && index < static_cast<int>(renderer.GetMaterialCount())) { return true; }
	ThrowException(std::format("IndexOutOfRangeException: material の番号が範囲外です ({} / materialCount {})", index, renderer.GetMaterialCount()));
	return false;
}

void SetCanvasMaterial(ObjectHandle *material, int index, ObjectHandle *self) {
	auto *renderer = Resolve<CanvasRenderer>(self);
	if (!renderer || !CheckMaterialIndex(*renderer, index)) { return; }

	std::shared_ptr<Material> shared;
	if (ResolveSharedAsset(material, shared)) { renderer->SetMaterial(std::move(shared), static_cast<uint32_t>(index)); }
}

ObjectHandle *GetCanvasMaterial(int index, ObjectHandle *self) {
	auto *renderer = Resolve<CanvasRenderer>(self);
	if (!renderer || !CheckMaterialIndex(*renderer, index)) { return nullptr; }
	return MakeHandle(renderer->GetMaterial(static_cast<uint32_t>(index)).get());
}


// ---- Sprite ----

// Unity の Sprite.Create。アセットではないので保存されない
ObjectHandle *CreateSprite(ObjectHandle *texture, const Rect &rect, const Vector2 &pivot) {
	if (!texture) {
		ThrowException("ArgumentNullException: texture が null です");
		return nullptr;
	}
	std::shared_ptr<Texture> shared;
	if (!ResolveSharedAsset(texture, shared)) { return nullptr; }
	return MakeOwningHandle(Sprite::Create(std::move(shared), rect, pivot));
}


// ---- ColorBlock（Unity の値の型。Selectable.colors） ----

void ConstructColorBlock(void *memory) { new (memory) ColorBlock(); }


// ---- Graphic・Text ----

void CrossFadeColor(const Color &targetColor, float duration, bool ignoreTimeScale, bool useAlpha, ObjectHandle *self) {
	if (auto *graphic = Resolve<Graphic>(self)) { graphic->CrossFadeColor(targetColor, duration, ignoreTimeScale, useAlpha); }
}

std::string GetText(ObjectHandle *self) {
	auto *text = Resolve<Text>(self);
	return text ? text->GetText() : std::string();
}

void SetText(const std::string &value, ObjectHandle *self) {
	if (auto *text = Resolve<Text>(self)) { text->SetText(value); }
}

// ---- Selectable・Button・UnityEvent ----

// スクリプトの UnityEvent は、持ち主の Button の番号を持つ（Button が消えたら MissingReferenceException。持ち手と同じ考え）
struct ScriptUnityEvent {
	int refCount = 1;
	uint32_t ownerInstanceId = 0;
};

void AddRefUnityEvent(ScriptUnityEvent *self) {
	self->refCount++;
}

void ReleaseUnityEvent(ScriptUnityEvent *self) {
	if (--self->refCount == 0) { delete self; }
}

UnityEvent *ResolveUnityEvent(ScriptUnityEvent *self) {
	auto *button = dynamic_cast<Button *>(Object::FindObjectFromInstanceId(self->ownerInstanceId));
	if (!button) {
		ThrowException("MissingReferenceException: この UnityEvent の Button は破棄されています");
		return nullptr;
	}
	return &button->onClick_;
}

// スクリプトの関数を外すときの目印（delegate なら、つないだオブジェクトとメソッド。ふつうの関数なら関数そのもの）
std::pair<const void *, const void *> MakeListenerKey(asIScriptFunction *action) {
	if (auto *method = action->GetDelegateFunction()) { return {action->GetDelegateObject(), method}; }
	return {nullptr, action};
}

void RemoveAllUnityEventListeners(ScriptUnityEvent *self) {
	if (auto *event = ResolveUnityEvent(self)) { event->RemoveAllListeners(); }
}

void InvokeUnityEvent(ScriptUnityEvent *self) {
	if (auto *event = ResolveUnityEvent(self)) { event->Invoke(); }
}

int32_t GetPersistentEventCount(ScriptUnityEvent *self) {
	auto *event = ResolveUnityEvent(self);
	return event ? event->GetPersistentEventCount() : 0;
}

std::string GetPersistentMethodName(int32_t index, ScriptUnityEvent *self) {
	auto *event = ResolveUnityEvent(self);
	return event ? event->GetPersistentMethodName(index) : std::string();
}

ScriptUnityEvent *GetOnClick(ObjectHandle *self) {
	auto *button = Resolve<Button>(self);
	return button ? new ScriptUnityEvent{1, button->GetInstanceId()} : nullptr;
}

void RegisterGraphicMembers(const TypeRegistrar &g) {
	RegisterBehaviorMembers(g);
	g.Method("Color get_color() const property", asFUNCTION((Get<Graphic, const Color &, &Graphic::GetColor>)));
	g.Method("void set_color(const Color &in value) property", asFUNCTION((Set<Graphic, const Color &, &Graphic::SetColor>)));
	g.Method("bool get_raycastTarget() const property", asFUNCTION((Get<Graphic, bool, &Graphic::GetRaycastTarget>)));
	g.Method("void set_raycastTarget(bool value) property", asFUNCTION((Set<Graphic, bool, &Graphic::SetRaycastTarget>)));
	g.Method("void CrossFadeColor(const Color &in targetColor, float duration, bool ignoreTimeScale, bool useAlpha)", asFUNCTION(CrossFadeColor));
	g.Method("Material@ get_material() const property", asFUNCTION(GetGraphicMaterial));
	g.Method("void set_material(Material@+ value) property", asFUNCTION((SetAssetHandle<Graphic, Material, &Graphic::SetMaterial>)));
	g.Method("CanvasRenderer@ get_canvasRenderer() const property", asFUNCTION((GetHandle<Graphic, CanvasRenderer, &Graphic::GetCanvasRenderer>)));
	g.Method("void SetAllDirty()", asFUNCTION((Call<Graphic, &Graphic::SetAllDirty>)));
	g.Method("void SetVerticesDirty()", asFUNCTION((Call<Graphic, &Graphic::SetVerticesDirty>)));
	g.Method("void SetMaterialDirty()", asFUNCTION((Call<Graphic, &Graphic::SetMaterialDirty>)));
}

void RegisterSelectableMembers(const TypeRegistrar &s) {
	RegisterBehaviorMembers(s);
	s.Method("bool get_interactable() const property", asFUNCTION((Get<Selectable, bool, &Selectable::IsInteractable>)));
	s.Method("void set_interactable(bool value) property", asFUNCTION((Set<Selectable, bool, &Selectable::SetInteractable>)));
	s.Method("Graphic@ get_targetGraphic() const property", asFUNCTION((GetHandle<Selectable, Graphic, &Selectable::GetTargetGraphic>)));
	s.Method("void set_targetGraphic(Graphic@+ value) property", asFUNCTION(SetTargetGraphic));
	s.Method("ColorBlock get_colors() const property", asFUNCTION((Get<Selectable, const ColorBlock &, &Selectable::GetColors>)));
	s.Method("void set_colors(const ColorBlock &in value) property", asFUNCTION((Set<Selectable, const ColorBlock &, &Selectable::SetColors>)));
	s.Method("Selectable@ FindSelectable(const Vector3 &in dir) const", asFUNCTION(FindSelectable));
	s.Method("Selectable@ FindSelectableOnLeft() const", asFUNCTION((GetHandle<Selectable, Selectable, &Selectable::FindSelectableOnLeft>)));
	s.Method("Selectable@ FindSelectableOnRight() const", asFUNCTION((GetHandle<Selectable, Selectable, &Selectable::FindSelectableOnRight>)));
	s.Method("Selectable@ FindSelectableOnUp() const", asFUNCTION((GetHandle<Selectable, Selectable, &Selectable::FindSelectableOnUp>)));
	s.Method("Selectable@ FindSelectableOnDown() const", asFUNCTION((GetHandle<Selectable, Selectable, &Selectable::FindSelectableOnDown>)));
	s.Method("void Select()", asFUNCTION((Call<Selectable, &Selectable::Select>)));
}

} // namespace



ScriptBindingsInternal::TypeRegistrar ou::ScriptBindingsInternal::RegisterAssetType(asIScriptEngine *engine, const char *type) {
	const TypeRegistrar a{engine, type};
	a.Method("string get_name() const property", asFUNCTION(GetAssetName));
	a.Method("void set_name(const string &in value) property", asFUNCTION(SetAssetName));
	a.Method("int GetInstanceID() const", asFUNCTION(GetAssetInstanceID));
	return a;
}


void ou::ScriptBindingsInternal::RegisterRendererMembers(const TypeRegistrar &r) {
	RegisterBehaviorMembers(r);
	r.Method("Material@ get_material() property", asFUNCTION(GetMaterial));
	r.Method("void set_material(Material@+ value) property", asFUNCTION(SetSharedMaterial));
	r.Method("Material@ get_sharedMaterial() const property", asFUNCTION(GetSharedMaterial));
	r.Method("void set_sharedMaterial(Material@+ value) property", asFUNCTION(SetSharedMaterial));
	r.Method("array<Material@>@ get_materials() property", asFUNCTION(GetMaterials));
	r.Method("void set_materials(array<Material@>@+ value) property", asFUNCTION(SetSharedMaterials));
	r.Method("array<Material@>@ get_sharedMaterials() const property", asFUNCTION(GetSharedMaterials));
	r.Method("void set_sharedMaterials(array<Material@>@+ value) property", asFUNCTION(SetSharedMaterials));
}



void ou::ScriptBindings::RegisterComponentMembers(asIScriptEngine *engine) {
	// ---- 値の型・enum（下のメソッドの宣言に出てくるので先に登録する） ----
	Check(engine->RegisterObjectType("ColorBlock", sizeof(ColorBlock), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_ALLFLOATS | asGetTypeTraits<ColorBlock>()), "ColorBlock");
	Check(engine->RegisterObjectBehaviour("ColorBlock", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructColorBlock), asCALL_CDECL_OBJLAST), "ColorBlock");
	Check(engine->RegisterObjectProperty("ColorBlock", "Color normalColor", asOFFSET(ColorBlock, normalColor)), "ColorBlock::normalColor");
	Check(engine->RegisterObjectProperty("ColorBlock", "Color highlightedColor", asOFFSET(ColorBlock, highlightedColor)), "ColorBlock::highlightedColor");
	Check(engine->RegisterObjectProperty("ColorBlock", "Color pressedColor", asOFFSET(ColorBlock, pressedColor)), "ColorBlock::pressedColor");
	Check(engine->RegisterObjectProperty("ColorBlock", "Color selectedColor", asOFFSET(ColorBlock, selectedColor)), "ColorBlock::selectedColor");
	Check(engine->RegisterObjectProperty("ColorBlock", "Color disabledColor", asOFFSET(ColorBlock, disabledColor)), "ColorBlock::disabledColor");
	Check(engine->RegisterObjectProperty("ColorBlock", "float colorMultiplier", asOFFSET(ColorBlock, colorMultiplier)), "ColorBlock::colorMultiplier");
	Check(engine->RegisterObjectProperty("ColorBlock", "float fadeDuration", asOFFSET(ColorBlock, fadeDuration)), "ColorBlock::fadeDuration");
	// Text は TextMeshPro に合わせた作りなので、TextMeshPro の名前で出す（TextAlignmentOptions::Center）
	RegisterEnum<TextAlignment>(engine, "TextAlignmentOptions");

	// ---- AudioClip ----
	{
		const auto a = RegisterAssetType(engine, "AudioClip");
		a.Method("float get_length() const property", asFUNCTION(GetClipLength));
		a.Method("int get_frequency() const property", asFUNCTION(GetClipFrequency));
		a.Method("int get_channels() const property", asFUNCTION(GetClipChannels));
	}

	// ---- Texture（Unity と同じく、Texture2D・RenderTexture の親） ----
	{
		const auto t = RegisterAssetType(engine, "Texture");
		t.Method("int get_width() const property", asFUNCTION(GetTextureWidth));
		t.Method("int get_height() const property", asFUNCTION(GetTextureHeight));
	}

	// ---- Texture2D ----
	{
		const auto t = RegisterAssetType(engine, "Texture2D");
		t.Method("int get_width() const property", asFUNCTION(GetTextureWidth));
		t.Method("int get_height() const property", asFUNCTION(GetTextureHeight));
		t.Method("bool get_isReadable() const property", asFUNCTION(IsTextureReadable));
	}

	// ---- RenderTexture ----
	RegisterEnum<RenderTextureFormat>(engine, "RenderTextureFormat");
	{
		const auto r = RegisterAssetType(engine, "RenderTexture");
		Check(engine->RegisterObjectBehaviour("RenderTexture", asBEHAVE_FACTORY, "RenderTexture@ f(int width, int height, int depth, RenderTextureFormat format = RenderTextureFormat::Default)", asFUNCTION(CreateRenderTexture), asCALL_CDECL), "RenderTexture factory");
		r.Method("int get_width() const property", asFUNCTION(GetTextureWidth));
		r.Method("int get_height() const property", asFUNCTION(GetTextureHeight));
		r.Method("RenderTextureFormat get_format() const property", asFUNCTION((GetEnum<RenderTexture, RenderTextureFormat, &RenderTexture::GetFormat>)));
		r.Method("bool get_enableRandomWrite() const property", asFUNCTION((Get<RenderTexture, bool, &RenderTexture::GetEnableRandomWrite>)));
		r.Method("void set_enableRandomWrite(bool value) property", asFUNCTION((Set<RenderTexture, bool, &RenderTexture::SetEnableRandomWrite>)));
		r.Method("bool Create()", asFUNCTION(CreateTextureResource));
		r.Method("void Release()", asFUNCTION((Call<RenderTexture, &RenderTexture::Release>)));
		r.Method("bool IsCreated() const", asFUNCTION((Get<RenderTexture, bool, &RenderTexture::IsCreated>)));
	}

	// ---- AudioSource ----
	{
		const auto a = RegisterComponentType(engine, "AudioSource");
		a.Method("void Play()", asFUNCTION(CallAudioSource<&AudioSource::Play>));
		a.Method("void Stop()", asFUNCTION(CallAudioSource<&AudioSource::Stop>));
		a.Method("void Pause()", asFUNCTION(CallAudioSource<&AudioSource::Pause>));
		a.Method("void UnPause()", asFUNCTION(CallAudioSource<&AudioSource::UnPause>));
		a.Method("void PlayOneShot(AudioClip@+ clip, float volumeScale = 1.0f)", asFUNCTION(PlayOneShot));
		a.Method("bool get_isPlaying() const property", asFUNCTION((Get<AudioSource, bool, &AudioSource::IsPlaying>)));
		a.Method("AudioClip@ get_clip() const property", asFUNCTION(GetClip));
		a.Method("void set_clip(AudioClip@+ value) property", asFUNCTION(SetClip));
		a.Method("float get_volume() const property", asFUNCTION((Get<AudioSource, float, &AudioSource::GetVolume>)));
		a.Method("void set_volume(float value) property", asFUNCTION((Set<AudioSource, float, &AudioSource::SetVolume>)));
		a.Method("float get_pitch() const property", asFUNCTION((Get<AudioSource, float, &AudioSource::GetPitch>)));
		a.Method("void set_pitch(float value) property", asFUNCTION((Set<AudioSource, float, &AudioSource::SetPitch>)));
		a.Method("bool get_loop() const property", asFUNCTION((Get<AudioSource, bool, &AudioSource::GetLoop>)));
		a.Method("void set_loop(bool value) property", asFUNCTION((Set<AudioSource, bool, &AudioSource::SetLoop>)));
		a.Method("bool get_mute() const property", asFUNCTION((Get<AudioSource, bool, &AudioSource::GetMute>)));
		a.Method("void set_mute(bool value) property", asFUNCTION((Set<AudioSource, bool, &AudioSource::SetMute>)));
		a.Method("bool get_playOnAwake() const property", asFUNCTION((Get<AudioSource, bool, &AudioSource::GetPlayOnAwake>)));
		a.Method("void set_playOnAwake(bool value) property", asFUNCTION((Set<AudioSource, bool, &AudioSource::SetPlayOnAwake>)));
	}

	// ---- AudioListener（Unity と同じく、音量は static） ----
	RegisterComponentType(engine, "AudioListener");
	engine->SetDefaultNamespace("AudioListener");
	Check(engine->RegisterGlobalFunction("float get_volume() property", asFUNCTION(AudioListener::GetVolume), asCALL_CDECL), "AudioListener::volume");
	Check(engine->RegisterGlobalFunction("void set_volume(float value) property", asFUNCTION(AudioListener::SetVolume), asCALL_CDECL), "AudioListener::volume");
	engine->SetDefaultNamespace("");

	// ---- Material ----
	{
		const auto m = RegisterAssetType(engine, "Material");
		Check(engine->RegisterObjectBehaviour("Material", asBEHAVE_FACTORY, "Material@ f(Shader@+ shader)", asFUNCTION(CreateMaterialFromShader), asCALL_CDECL), "Material factory");
		Check(engine->RegisterObjectBehaviour("Material", asBEHAVE_FACTORY, "Material@ f(Material@+ source)", asFUNCTION(CreateMaterialFromSource), asCALL_CDECL), "Material factory");
		m.Method("Shader@ get_shader() const property", asFUNCTION((GetAssetHandle<Material, Shader, &Material::GetShader>)));
		m.Method("void set_shader(Shader@+ value) property", asFUNCTION((SetAssetHandle<Material, Shader, &Material::SetShader>)));
		m.Method("int get_renderQueue() const property", asFUNCTION((Get<Material, int, &Material::GetRenderQueue>)));
		m.Method("void set_renderQueue(int value) property", asFUNCTION((Set<Material, int, &Material::SetRenderQueue>)));
		m.Method("Texture2D@ get_mainTexture() const property", asFUNCTION((GetAssetHandle<Material, Texture, &Material::GetMainTexture>)));
		m.Method("void set_mainTexture(Texture2D@+ value) property", asFUNCTION((SetAssetHandle<Material, Texture, &Material::SetMainTexture>)));
		m.Method("Vector4 GetVector(const string &in name) const", asFUNCTION(GetVector));
		m.Method("void SetVector(const string &in name, const Vector4 &in value)", asFUNCTION(SetVector));
		m.Method("Color get_color() const property", asFUNCTION(GetMaterialColor));
		m.Method("void set_color(const Color &in value) property", asFUNCTION(SetMaterialColor));
		m.Method("Vector2 get_mainTextureOffset() const property", asFUNCTION((Get<Material, const Vector2 &, &Material::GetUvOffset>)));
		m.Method("void set_mainTextureOffset(const Vector2 &in value) property", asFUNCTION((Set<Material, const Vector2 &, &Material::SetUvOffset>)));
		m.Method("Vector2 get_mainTextureScale() const property", asFUNCTION((Get<Material, const Vector2 &, &Material::GetUvScale>)));
		m.Method("void set_mainTextureScale(const Vector2 &in value) property", asFUNCTION((Set<Material, const Vector2 &, &Material::SetUvScale>)));
		m.Method("Color GetColor(const string &in name) const", asFUNCTION(GetNamedColor));
		m.Method("void SetColor(const string &in name, const Color &in value)", asFUNCTION(SetNamedColor));
		m.Method("float GetFloat(const string &in name) const", asFUNCTION(GetFloat));
		m.Method("void SetFloat(const string &in name, float value)", asFUNCTION(SetFloat));
		m.Method("bool HasProperty(const string &in name) const", asFUNCTION(HasProperty));
		m.Method("Texture@ GetTexture(const string &in name) const", asFUNCTION(GetNamedTexture));
		m.Method("void SetTexture(const string &in name, Texture@+ value)", asFUNCTION(SetNamedTexture));
	}

	// ---- Renderer・MeshRenderer（ほかの派生は ScriptBindingsRender.cpp） ----
	RegisterRendererMembers(RegisterComponentType(engine, "Renderer"));
	RegisterRendererMembers(RegisterComponentType(engine, "MeshRenderer"));

	// ---- Sprite・FontAsset ----
	{
		const auto s = RegisterAssetType(engine, "Sprite");
		s.Method("Texture2D@ get_texture() const property", asFUNCTION((GetAssetHandle<Sprite, Texture, &Sprite::GetTexture>)));
		s.Method("Rect get_rect() const property", asFUNCTION((Get<Sprite, const Rect &, &Sprite::GetRect>)));
		s.Method("Vector2 get_pivot() const property", asFUNCTION((Get<Sprite, const Vector2 &, &Sprite::GetPivot>)));
		engine->SetDefaultNamespace("Sprite");
		Check(engine->RegisterGlobalFunction("Sprite@ Create(Texture2D@+ texture, const Rect &in rect, const Vector2 &in pivot = Vector2(0.5f, 0.5f))", asFUNCTION(CreateSprite), asCALL_CDECL), "Sprite::Create");
		engine->SetDefaultNamespace("");
	}
	RegisterAssetType(engine, "FontAsset");

	// ---- Canvas・CanvasRenderer ----
	RegisterBehaviorMembers(RegisterComponentType(engine, "Canvas"));
	{
		const auto c = RegisterComponentType(engine, "CanvasRenderer");
		c.Method("Color GetColor() const", asFUNCTION((Get<CanvasRenderer, const Color &, &CanvasRenderer::GetColor>)));
		c.Method("void SetColor(const Color &in color)", asFUNCTION((Set<CanvasRenderer, const Color &, &CanvasRenderer::SetColor>)));
		c.Method("float GetAlpha() const", asFUNCTION((Get<CanvasRenderer, float, &CanvasRenderer::GetAlpha>)));
		c.Method("void SetAlpha(float alpha)", asFUNCTION((Set<CanvasRenderer, float, &CanvasRenderer::SetAlpha>)));
		c.Method("void Clear()", asFUNCTION((Call<CanvasRenderer, &CanvasRenderer::Clear>)));
		c.Method("int get_materialCount() const property", asFUNCTION(GetMaterialCount));
		c.Method("void set_materialCount(int value) property", asFUNCTION(SetMaterialCount));
		c.Method("void SetMaterial(Material@+ material, int index)", asFUNCTION(SetCanvasMaterial));
		c.Method("Material@ GetMaterial(int index = 0) const", asFUNCTION(GetCanvasMaterial));
		c.Method("void SetTexture(Texture2D@+ texture)", asFUNCTION((SetAssetHandle<CanvasRenderer, Texture, &CanvasRenderer::SetTexture>)));
	}

	// ---- Graphic・Image・Text ----
	RegisterGraphicMembers(RegisterComponentType(engine, "Graphic"));
	{
		const auto i = RegisterComponentType(engine, "Image");
		RegisterGraphicMembers(i);
		i.Method("Sprite@ get_sprite() const property", asFUNCTION((GetAssetHandle<Image, Sprite, &Image::GetSprite>)));
		i.Method("void set_sprite(Sprite@+ value) property", asFUNCTION((SetAssetHandle<Image, Sprite, &Image::SetSprite>)));
		i.Method("void SetNativeSize()", asFUNCTION((Call<Image, &Image::SetNativeSize>)));
	}
	{
		const auto t = RegisterComponentType(engine, "Text");
		RegisterGraphicMembers(t);
		t.Method("string get_text() const property", asFUNCTION(GetText));
		t.Method("void set_text(const string &in value) property", asFUNCTION(SetText));
		t.Method("FontAsset@ get_font() const property", asFUNCTION((GetAssetHandle<Text, FontAsset, &Text::GetFont>)));
		t.Method("void set_font(FontAsset@+ value) property", asFUNCTION((SetAssetHandle<Text, FontAsset, &Text::SetFont>)));
		t.Method("float get_fontSize() const property", asFUNCTION((Get<Text, float, &Text::GetFontSize>)));
		t.Method("void set_fontSize(float value) property", asFUNCTION((Set<Text, float, &Text::SetFontSize>)));
		t.Method("TextAlignmentOptions get_alignment() const property", asFUNCTION((GetEnum<Text, TextAlignment, &Text::GetAlignment>)));
		t.Method("void set_alignment(TextAlignmentOptions value) property", asFUNCTION((SetEnum<Text, TextAlignment, &Text::SetAlignment>)));
		t.Method("bool get_enableWordWrapping() const property", asFUNCTION((Get<Text, bool, &Text::IsWordWrappingEnabled>)));
		t.Method("void set_enableWordWrapping(bool value) property", asFUNCTION((Set<Text, bool, &Text::SetWordWrappingEnabled>)));
		t.Method("float get_lineSpacing() const property", asFUNCTION((Get<Text, float, &Text::GetLineSpacing>)));
		t.Method("void set_lineSpacing(float value) property", asFUNCTION((Set<Text, float, &Text::SetLineSpacing>)));
		t.Method("float get_characterSpacing() const property", asFUNCTION((Get<Text, float, &Text::GetCharacterSpacing>)));
		t.Method("void set_characterSpacing(float value) property", asFUNCTION((Set<Text, float, &Text::SetCharacterSpacing>)));
		t.Method("Color get_outlineColor() const property", asFUNCTION((Get<Text, const Color &, &Text::GetOutlineColor>)));
		t.Method("void set_outlineColor(const Color &in value) property", asFUNCTION((Set<Text, const Color &, &Text::SetOutlineColor>)));
		t.Method("float get_outlineWidth() const property", asFUNCTION((Get<Text, float, &Text::GetOutlineWidth>)));
		t.Method("void set_outlineWidth(float value) property", asFUNCTION((Set<Text, float, &Text::SetOutlineWidth>)));
	}

	// ---- UnityEvent ----
	Check(engine->RegisterFuncdef("void UnityAction()"), "UnityAction");
	{
		const TypeRegistrar e{engine, "UnityEvent"};
		Check(engine->RegisterObjectType("UnityEvent", 0, asOBJ_REF), "UnityEvent");
		e.Behaviour(asBEHAVE_ADDREF, "void f()", asFUNCTION(AddRefUnityEvent));
		e.Behaviour(asBEHAVE_RELEASE, "void f()", asFUNCTION(ReleaseUnityEvent));
		e.Method("void AddListener(UnityAction@ call)", asFUNCTION(AddUnityEventListener));
		e.Method("void RemoveListener(UnityAction@ call)", asFUNCTION(RemoveUnityEventListener));
		e.Method("void RemoveAllListeners()", asFUNCTION(RemoveAllUnityEventListeners));
		e.Method("void Invoke()", asFUNCTION(InvokeUnityEvent));
		e.Method("int GetPersistentEventCount() const", asFUNCTION(GetPersistentEventCount));
		e.Method("string GetPersistentMethodName(int index) const", asFUNCTION(GetPersistentMethodName));
	}

	// ---- Selectable・Button ----
	RegisterSelectableMembers(RegisterComponentType(engine, "Selectable"));
	{
		const auto b = RegisterComponentType(engine, "Button");
		RegisterSelectableMembers(b);
		b.Method("UnityEvent@ get_onClick() const property", asFUNCTION(GetOnClick));
	}
	engine->SetDefaultNamespace("Selectable");
	Check(engine->RegisterGlobalFunction("array<Selectable@>@ get_allSelectablesArray() property", asFUNCTION(GetAllSelectables), asCALL_CDECL), "Selectable::allSelectablesArray");
	engine->SetDefaultNamespace("");
}


void ou::ScriptBindings::AddUnityEventListener(asIScriptFunction *action, void *self) {
	if (!action) { return; }

	auto *event = ResolveUnityEvent(static_cast<ScriptUnityEvent *>(self));
	if (!event) {
		action->Release();
		return;
	}

	// 受け取った参照は、つないでいるあいだ持ち、外したとき（Button が消えたときも）に手放す
	const auto key = MakeListenerKey(action);
	std::shared_ptr<asIScriptFunction> holder(action, [](asIScriptFunction *function) { function->Release(); });
	event->AddListener([holder] { ScriptEngine::CallMethodWith(nullptr, holder.get(), [](asIScriptContext *) {}); }, key);
}


void ou::ScriptBindings::RemoveUnityEventListener(asIScriptFunction *action, void *self) {
	if (!action) { return; }

	if (auto *event = ResolveUnityEvent(static_cast<ScriptUnityEvent *>(self))) { event->RemoveListener(MakeListenerKey(action)); }
	action->Release();
}
