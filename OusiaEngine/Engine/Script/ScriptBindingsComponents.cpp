#include "ScriptBindings.h"
#include "Asset/AssetDatabase.h"
#include "Audio/AudioClip.h"
#include "Audio/AudioSource.h"
#include "Math/Color.h"
#include "Math/Vector2.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Texture.h"
#include "Render/Renderer.h"
#include "Script/ScriptBindingsInternal.h"
#include "Script/ScriptEngine.h"
#include "UI/Button.h"
#include "UI/Graphic.h"
#include "UI/Image.h"
#include "UI/Text.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include <angelscript.h>

using namespace ou;
using namespace ou::ScriptBindingsInternal;


namespace {

// C++ はアセットを shared_ptr で持つが、持ち手はポインタしか引けないので、アセットのデータベースから引き直す
// アセットでないもの（Renderer が複製した Material など）は、持っている Component から渡す
template <typename T>
std::shared_ptr<T> FindSharedAsset(T *asset) {
	if (!asset) { return nullptr; }
	const auto reference = AssetDatabase::FindAssetReference(asset);
	if (!reference) { return nullptr; }
	return std::dynamic_pointer_cast<T>(AssetDatabase::LoadAsset(reference->guid, reference->subAsset));
}

// null を渡せば外す。アセットでないものは渡せない
template <typename T>
bool ResolveSharedAsset(ObjectHandle *handle, std::shared_ptr<T> &out) {
	out = nullptr;
	if (!handle) { return true; }

	auto *asset = Resolve<T>(handle);
	if (!asset) { return false; }
	out = FindSharedAsset(asset);
	if (!out) {
		ThrowException("ArgumentException: アセットとして保存されていないものは渡せません");
		return false;
	}
	return true;
}


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
}

} // namespace



void ou::ScriptBindings::RegisterComponentMembers(asIScriptEngine *engine) {
	// ---- AudioClip ----
	{
		const TypeRegistrar a{engine, "AudioClip"};
		a.Method("string get_name() const property", asFUNCTION(GetAssetName));
		a.Method("void set_name(const string &in value) property", asFUNCTION(SetAssetName));
		a.Method("int GetInstanceID() const", asFUNCTION(GetAssetInstanceID));
		a.Method("float get_length() const property", asFUNCTION(GetClipLength));
		a.Method("int get_frequency() const property", asFUNCTION(GetClipFrequency));
		a.Method("int get_channels() const property", asFUNCTION(GetClipChannels));
	}

	// ---- Texture2D ----
	{
		const TypeRegistrar t{engine, "Texture2D"};
		t.Method("string get_name() const property", asFUNCTION(GetAssetName));
		t.Method("void set_name(const string &in value) property", asFUNCTION(SetAssetName));
		t.Method("int GetInstanceID() const", asFUNCTION(GetAssetInstanceID));
		t.Method("int get_width() const property", asFUNCTION(GetTextureWidth));
		t.Method("int get_height() const property", asFUNCTION(GetTextureHeight));
		t.Method("bool get_isReadable() const property", asFUNCTION(IsTextureReadable));
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

	// ---- Material ----
	{
		const TypeRegistrar m{engine, "Material"};
		m.Method("string get_name() const property", asFUNCTION(GetAssetName));
		m.Method("void set_name(const string &in value) property", asFUNCTION(SetAssetName));
		m.Method("int GetInstanceID() const", asFUNCTION(GetAssetInstanceID));
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
	}

	// ---- Renderer・MeshRenderer ----
	for (const auto *type : {"Renderer", "MeshRenderer"}) {
		const auto r = RegisterComponentType(engine, type);
		RegisterBehaviorMembers(r);
		r.Method("Material@ get_material() property", asFUNCTION(GetMaterial));
		r.Method("void set_material(Material@+ value) property", asFUNCTION(SetSharedMaterial));
		r.Method("Material@ get_sharedMaterial() const property", asFUNCTION(GetSharedMaterial));
		r.Method("void set_sharedMaterial(Material@+ value) property", asFUNCTION(SetSharedMaterial));
	}

	// ---- Graphic・Image・Text ----
	RegisterGraphicMembers(RegisterComponentType(engine, "Graphic"));
	RegisterGraphicMembers(RegisterComponentType(engine, "Image"));
	{
		const auto t = RegisterComponentType(engine, "Text");
		RegisterGraphicMembers(t);
		t.Method("string get_text() const property", asFUNCTION(GetText));
		t.Method("void set_text(const string &in value) property", asFUNCTION(SetText));
		t.Method("float get_fontSize() const property", asFUNCTION((Get<Text, float, &Text::GetFontSize>)));
		t.Method("void set_fontSize(float value) property", asFUNCTION((Set<Text, float, &Text::SetFontSize>)));
		t.Method("float get_lineSpacing() const property", asFUNCTION((Get<Text, float, &Text::GetLineSpacing>)));
		t.Method("void set_lineSpacing(float value) property", asFUNCTION((Set<Text, float, &Text::SetLineSpacing>)));
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
	const auto selectableMembers = [](const TypeRegistrar &s) {
		s.Method("bool get_interactable() const property", asFUNCTION((Get<Selectable, bool, &Selectable::IsInteractable>)));
		s.Method("void set_interactable(bool value) property", asFUNCTION((Set<Selectable, bool, &Selectable::SetInteractable>)));
	};
	selectableMembers(RegisterComponentType(engine, "Selectable"));
	{
		const auto b = RegisterComponentType(engine, "Button");
		selectableMembers(b);
		b.Method("UnityEvent@ get_onClick() const property", asFUNCTION(GetOnClick));
	}
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
