#include "ScriptBindings.h"
#include "Asset/AssetDatabase.h"
#include "Audio/AudioClip.h"
#include "Audio/AudioSource.h"
#include "Core/Object.h"
#include "Math/Color.h"
#include "Math/Mathf.h"
#include "Math/Quaternion.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Physics/BoxCollider.h"
#include "Physics/CapsuleCollider.h"
#include "Physics/CharacterController.h"
#include "Physics/Collider.h"
#include "Physics/Rigidbody.h"
#include "Physics/SphereCollider.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Texture.h"
#include "Render/MeshRenderer.h"
#include "Render/Renderer.h"
#include "Scene/Behavior.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/TagManager.h"
#include "Scene/Transform.h"
#include "Script/MonoBehavior.h"
#include "Script/MonoScript.h"
#include "Script/ScriptBindingsInternal.h"
#include "Serialize/Serializer.h"
#include "Serialize/TypeRegistry.h"
#include "UI/Button.h"
#include "UI/Graphic.h"
#include "UI/Image.h"
#include "UI/Text.h"

#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <angelscript.h>
#include <scriptarray/scriptarray.h>

using namespace ou;
using namespace ou::ScriptBindingsInternal;


namespace {

// ---- 持ち手 ----

// 同じオブジェクトには同じ持ち手を返す（スクリプトの a is b が成り立つように）
std::unordered_map<uint32_t, ObjectHandle *> &Handles() {
	static std::unordered_map<uint32_t, ObjectHandle *> handles;
	return handles;
}

// 消えたあとの例外の文面に使う型の名前（表は子の型が先に並ぶので、最初に当てはまったもの）
const char *DescribeType(const Object *object) {
	for (const auto &type : NativeTypes()) {
		if (type.isInstance(object)) { return type.name; }
	}
	return "Object";
}


// ---- Object ----

std::string GetName(ObjectHandle *self) {
	auto *object = Resolve<Object>(self);
	return object ? object->GetName() : std::string();
}

void SetName(const std::string &name, ObjectHandle *self) {
	if (auto *object = Resolve<Object>(self)) { object->SetName(name); }
}

int GetInstanceID(ObjectHandle *self) {
	return static_cast<int>(self->instanceId);
}

// Unity と同じく null を渡しても何もしない
void Destroy(ObjectHandle *handle) {
	if (!handle) { return; }
	if (auto *object = Resolve<Object>(handle)) { Object::Destroy(object); }
}

// ---- Instantiate（null は C++ がログに出す） ----

ObjectHandle *Instantiate(ObjectHandle *original) {
	auto *gameObject = original ? Resolve<GameObject>(original) : nullptr;
	return MakeHandle(Object::Instantiate(gameObject));
}

ObjectHandle *InstantiateWithParent(ObjectHandle *original, ObjectHandle *parent, bool instantiateInWorldSpace) {
	auto *gameObject = original ? Resolve<GameObject>(original) : nullptr;
	auto *parentTransform = parent ? Resolve<Transform>(parent) : nullptr;
	return MakeHandle(Object::Instantiate(gameObject, parentTransform, instantiateInWorldSpace));
}

ObjectHandle *InstantiateAt(ObjectHandle *original, const Vector3 &position, const Quaternion &rotation, ObjectHandle *parent) {
	auto *gameObject = original ? Resolve<GameObject>(original) : nullptr;
	auto *parentTransform = parent ? Resolve<Transform>(parent) : nullptr;
	return MakeHandle(Object::Instantiate(gameObject, position, rotation, parentTransform));
}

// GameObject か Component を渡すと、その GameObject を次のシーンを読み込んでも残す（Unity と同じくルートの GameObject だけ）
void DontDestroyOnLoad(ObjectHandle *handle) {
	if (!handle) { return; }
	if (auto *object = Resolve<Object>(handle)) { Object::DontDestroyOnLoad(object); }
}


// ---- GameObject ----

// Unity の new GameObject("name")。今のシーンに作る
ObjectHandle *CreateGameObject(const std::string &name) {
	auto *scene = SceneManager::GetActiveScene();
	if (!scene) {
		ThrowException("シーンがないため GameObject を作れません");
		return nullptr;
	}
	// 工場関数は参照を 1 つ持たせて返す。MakeHandle も 1 つ持たせて返すので、そのまま渡す
	return MakeHandle(scene->CreateGameObject(name));
}


// ---- Component ----

// Unity と同じく、Component の name は GameObject の名前
std::string GetComponentName(ObjectHandle *self) {
	auto *component = Resolve<Component>(self);
	return component && component->GetGameObject() ? component->GetGameObject()->GetName() : std::string();
}

void SetComponentName(const std::string &name, ObjectHandle *self) {
	auto *component = Resolve<Component>(self);
	if (component && component->GetGameObject()) { component->GetGameObject()->SetName(name); }
}

ObjectHandle *GetComponentGameObject(ObjectHandle *self) {
	auto *component = Resolve<Component>(self);
	return component ? MakeHandle(component->GetGameObject()) : nullptr;
}

ObjectHandle *GetComponentTransform(ObjectHandle *self) {
	auto *component = Resolve<Component>(self);
	return component ? MakeHandle(component->GetTransform()) : nullptr;
}


// ---- tag（GameObject と Component の両方。Component は GameObject のタグ） ----

// Unity と同じく、Tags にないタグを使うと例外にする
bool CheckTagDefined(const std::string &tag) {
	if (TagManager::IsDefined(tag)) { return true; }
	ThrowException(std::format("UnityException: Tag: {} is not defined. (Project Settings の Tags and Layers で足してください)", tag));
	return false;
}

template <typename T>
GameObject *ResolveTagOwner(ObjectHandle *self) {
	if constexpr (std::is_same_v<T, GameObject>) {
		return Resolve<GameObject>(self);
	} else {
		auto *component = Resolve<Component>(self);
		return component ? component->GetGameObject() : nullptr;
	}
}

template <typename T>
std::string GetTag(ObjectHandle *self) {
	auto *gameObject = ResolveTagOwner<T>(self);
	return gameObject ? gameObject->GetTag() : std::string();
}

template <typename T>
void SetTag(const std::string &tag, ObjectHandle *self) {
	auto *gameObject = ResolveTagOwner<T>(self);
	if (gameObject && CheckTagDefined(tag)) { gameObject->SetTag(tag); }
}

template <typename T>
bool CompareTag(const std::string &tag, ObjectHandle *self) {
	auto *gameObject = ResolveTagOwner<T>(self);
	return gameObject && CheckTagDefined(tag) && gameObject->GetTag() == tag;
}

ObjectHandle *FindWithTag(const std::string &tag) {
	if (!CheckTagDefined(tag)) { return nullptr; }
	return MakeHandle(GameObject::FindWithTag(tag));
}

CScriptArray *FindGameObjectsWithTag(const std::string &tag) {
	if (!CheckTagDefined(tag)) { return nullptr; }

	const auto found = GameObject::FindGameObjectsWithTag(tag);
	auto *type = asGetActiveContext()->GetEngine()->GetTypeInfoByDecl("array<GameObject@>");
	auto *array = CScriptArray::Create(type, static_cast<asUINT>(found.size()));
	for (size_t i = 0; i < found.size(); i++) {
		// MakeHandle が持たせた参照を、そのまま配列に持たせる
		*static_cast<void **>(array->At(static_cast<asUINT>(i))) = MakeHandle(found[i]);
	}
	return array;
}


// ---- Transform（スクリプトは Unity と同じく度、エンジンはラジアン） ----

enum class Space { World = 0, Self = 1 };

// Unity と同じく 0〜360 にそろえる
float Wrap360(float degrees) {
	// 0 のわずかに下は Repeat で 360 になってしまうので 0 にする
	const auto wrapped = Mathf::Repeat(degrees, 360.0f);
	return 360.0f - wrapped < 1e-4f ? 0.0f : wrapped;
}

Vector3 ToDegrees360(const Vector3 &radians) {
	const auto degrees = radians * Mathf::Rad2Deg;
	return {Wrap360(degrees.x), Wrap360(degrees.y), Wrap360(degrees.z)};
}

Quaternion EulerDegrees(const Vector3 &degrees) {
	return Quaternion::Euler(degrees * Mathf::Deg2Rad);
}

Vector3 GetEulerAngles(ObjectHandle *self) {
	auto *transform = Resolve<Transform>(self);
	return transform ? ToDegrees360(transform->GetWorldRotation().EulerAngles()) : Vector3::Zero;
}

void SetEulerAngles(const Vector3 &degrees, ObjectHandle *self) {
	if (auto *transform = Resolve<Transform>(self)) { transform->SetWorldRotation(EulerDegrees(degrees)); }
}

Vector3 GetLocalEulerAngles(ObjectHandle *self) {
	auto *transform = Resolve<Transform>(self);
	return transform ? ToDegrees360(transform->GetLocalEulerAngles()) : Vector3::Zero;
}

void SetLocalEulerAngles(const Vector3 &degrees, ObjectHandle *self) {
	if (auto *transform = Resolve<Transform>(self)) { transform->SetLocalEulerAngles(degrees * Mathf::Deg2Rad); }
}

Vector3 GetRight(ObjectHandle *self) {
	auto *transform = Resolve<Transform>(self);
	return transform ? transform->GetWorldRotation() * Vector3::Right : Vector3::Right;
}

Vector3 GetUp(ObjectHandle *self) {
	auto *transform = Resolve<Transform>(self);
	return transform ? transform->GetWorldRotation() * Vector3::Up : Vector3::Up;
}

ObjectHandle *GetParent(ObjectHandle *self) {
	auto *transform = Resolve<Transform>(self);
	return transform ? MakeHandle(transform->GetParent()) : nullptr;
}

// Unity の transform.parent = ... と同じく、ワールドの位置を保つ
void SetParent(ObjectHandle *parent, ObjectHandle *self) {
	auto *transform = Resolve<Transform>(self);
	if (!transform) { return; }

	Transform *parentTransform = nullptr;
	if (parent) {
		parentTransform = Resolve<Transform>(parent);
		if (!parentTransform) { return; }
	}
	transform->SetParent(parentTransform, true);
}

int GetChildCount(ObjectHandle *self) {
	auto *transform = Resolve<Transform>(self);
	return transform ? static_cast<int>(transform->GetChildren().size()) : 0;
}

ObjectHandle *GetChild(int index, ObjectHandle *self) {
	auto *transform = Resolve<Transform>(self);
	if (!transform) { return nullptr; }

	const auto &children = transform->GetChildren();
	if (index < 0 || index >= static_cast<int>(children.size())) {
		ThrowException(std::format("UnityException: Transform child out of bounds ({} / {})", index, children.size()));
		return nullptr;
	}
	return MakeHandle(children[index]);
}

ObjectHandle *Find(const std::string &path, ObjectHandle *self) {
	auto *transform = Resolve<Transform>(self);
	return transform ? MakeHandle(transform->Find(path)) : nullptr;
}

// Unity の既定は Space.Self（自分の向きで動かす）
void Translate(const Vector3 &translation, int relativeTo, ObjectHandle *self) {
	auto *transform = Resolve<Transform>(self);
	if (!transform) { return; }

	const auto delta = relativeTo == static_cast<int>(Space::Self) ? transform->GetWorldRotation() * translation : translation;
	transform->SetWorldPosition(transform->GetWorldPosition() + delta);
}

void TranslateXYZ(float x, float y, float z, int relativeTo, ObjectHandle *self) {
	Translate({x, y, z}, relativeTo, self);
}

void Rotate(const Vector3 &eulers, int relativeTo, ObjectHandle *self) {
	auto *transform = Resolve<Transform>(self);
	if (!transform) { return; }

	const auto rotation = EulerDegrees(eulers);
	if (relativeTo == static_cast<int>(Space::Self)) {
		transform->SetLocalRotation(transform->GetLocalRotation() * rotation);
	} else {
		transform->SetWorldRotation(rotation * transform->GetWorldRotation());
	}
}

void RotateXYZ(float x, float y, float z, int relativeTo, ObjectHandle *self) {
	Rotate({x, y, z}, relativeTo, self);
}

void LookAt(const Vector3 &worldPosition, const Vector3 &worldUp, ObjectHandle *self) {
	auto *transform = Resolve<Transform>(self);
	if (!transform) { return; }

	const auto direction = worldPosition - transform->GetWorldPosition();
	if (direction.LengthSquared() < 1e-12f) { return; }
	transform->SetWorldRotation(LookRotation(direction, worldUp));
}


// ---- 値の型の id（変数の保存で型を見分けるのに使う。登録したあとに引いて覚えておく） ----

struct ValueTypeIds {
	int string = 0;
	int vector2 = 0;
	int vector3 = 0;
	int quaternion = 0;
	int color = 0;
};

ValueTypeIds &TypeIds() {
	static ValueTypeIds ids;
	return ids;
}


// ---- 持ち手で渡す型の表を作る手伝い ----

// 持ち手の変数は、C++ のポインタにして Serializer::Reference に渡す（fileId で保存され、Inspector でつなげる）
template <typename T>
void SerializeHandle(Serializer &serializer, std::string_view name, ObjectHandle *&handle) {
	T *object = handle ? dynamic_cast<T *>(Object::FindObjectFromInstanceId(handle->instanceId)) : nullptr;
	const auto *before = object;

	// 書くときは object が変わらないので、何もしない
	serializer.Reference(name, object);
	if (object == before) { return; }

	if (handle) { Release(handle); }
	handle = MakeHandle(object);
}

// アセットの型（Material・AudioClip・Texture2D）の持ち手は、Serializer::Asset に渡す（guid で保存され、Inspector では Project から落とせる・ピッカーで選べる）
// 持ち手がアセットの shared_ptr を持っているので、それをそのまま渡す。実行中に作ったもの（renderer.material の複製など）はアセットではないので保存しない
template <typename T>
void SerializeAssetHandle(Serializer &serializer, std::string_view name, ObjectHandle *&handle) {
	auto asset = handle ? std::dynamic_pointer_cast<T>(handle->asset) : nullptr;
	const auto *before = asset.get();

	// 書くときは asset が変わらないので、何もしない
	serializer.Asset(name, asset);
	if (asset.get() == before) { return; }

	if (handle) { Release(handle); }
	handle = MakeHandle(asset.get()); // アセットなら、持ち手が shared_ptr も持つ
}

template <typename T>
bool IsInstance(const Object *object) {
	return dynamic_cast<const T *>(object) != nullptr;
}

template <typename T>
Component *FindComponent(const GameObject *gameObject) {
	return gameObject->GetComponent<T>();
}

// canAdd: AddComponent<T> で作れるか（抽象クラスと Transform は作れない）
template <typename T>
NativeType MakeNativeType(const char *name, const char *parent, bool canAdd = false) {
	NativeType type{name, parent, &IsInstance<T>, asFUNCTION(ScriptBindingsInternal::Downcast<T>), nullptr, nullptr, canAdd};
	if constexpr (std::is_base_of_v<Component, T>) {
		type.find = &FindComponent<T>;
	}
	if constexpr (std::is_base_of_v<Component, T> || std::is_same_v<T, GameObject>) {
		type.serialize = &SerializeHandle<T>;
	} else if constexpr (std::is_same_v<T, Material> || std::is_same_v<T, AudioClip> || std::is_same_v<T, Texture>) {
		type.serialize = &SerializeAssetHandle<T>;
	}
	return type;
}

const NativeType *FindNativeTypeByName(std::string_view name) {
	for (const auto &type : NativeTypes()) {
		if (name == type.name) { return &type; }
	}
	return nullptr;
}

// 親・親の親…の順
std::vector<const char *> CollectAncestors(const NativeType &type) {
	std::vector<const char *> ancestors;
	for (const auto *parent = type.parent; parent;) {
		ancestors.push_back(parent);
		const auto *row = FindNativeTypeByName(parent);
		parent = row ? row->parent : nullptr;
	}
	return ancestors;
}

// GameObject ならそれ、Component なら付いている GameObject
GameObject *ResolveOwnerGameObject(ObjectHandle *self) {
	auto *object = Resolve<Object>(self);
	if (auto *gameObject = dynamic_cast<GameObject *>(object)) { return gameObject; }
	if (auto *component = dynamic_cast<Component *>(object)) { return component->GetGameObject(); }
	return nullptr;
}

// Unity と同じく、スクリプトのクラスはファイル名＝クラス名の .as から探す
std::shared_ptr<MonoScript> FindMonoScript(const std::string &className) {
	for (const auto &guid : AssetDatabase::FindAssets(std::format("t:MonoScript {}", className))) {
		const auto path = AssetDatabase::GuidToAssetPath(guid);
		const auto slash = path.find_last_of('/');
		const auto fileName = slash == std::string::npos ? path : path.substr(slash + 1);
		if (fileName == className + ".as") { return AssetDatabase::LoadAssetAtPath<MonoScript>(path); }
	}
	return nullptr;
}

} // namespace



// ---- ScriptBindingsInternal（ヘッダで宣言したもの） ----

ScriptBindingsInternal::ObjectHandle *ou::ScriptBindingsInternal::MakeHandle(Object *object) {
	if (!object) { return nullptr; }

	auto &handles = Handles();
	if (const auto it = handles.find(object->GetInstanceId()); it != handles.end()) {
		it->second->refCount++;
		return it->second;
	}

	auto *handle = new ObjectHandle{1, object->GetInstanceId(), DescribeType(object), nullptr};

	// アセットはアセットのデータベースが shared_ptr で持ち、誰も持たなくなると UnloadUnusedAssets で消える
	// スクリプトの変数に入れたプレハブが消えないよう、持ち手も同じ shared_ptr を持つ
	if (const auto reference = AssetDatabase::FindAssetReference(object)) {
		auto asset = AssetDatabase::LoadAsset(reference->guid, reference->subAsset);
		if (asset.get() == object) { handle->asset = std::move(asset); }
	}

	handles.emplace(handle->instanceId, handle);
	return handle;
}


void ou::ScriptBindingsInternal::Release(ObjectHandle *self) {
	if (--self->refCount > 0) { return; }
	Handles().erase(self->instanceId);
	delete self;
}


std::vector<NativeType> &ou::ScriptBindingsInternal::NativeTypes() {
	// 子の型を親より先に並べる
	static std::vector<NativeType> types = {
		MakeNativeType<GameObject>("GameObject", "Object"),
		MakeNativeType<Transform>("Transform", "Component"),
		MakeNativeType<CharacterController>("CharacterController", "Collider", true),
		MakeNativeType<BoxCollider>("BoxCollider", "Collider", true),
		MakeNativeType<SphereCollider>("SphereCollider", "Collider", true),
		MakeNativeType<CapsuleCollider>("CapsuleCollider", "Collider", true),
		MakeNativeType<Collider>("Collider", "Behavior"),
		MakeNativeType<Rigidbody>("Rigidbody", "Component", true),
		MakeNativeType<AudioSource>("AudioSource", "Component", true),
		MakeNativeType<MeshRenderer>("MeshRenderer", "Renderer", true),
		MakeNativeType<Renderer>("Renderer", "Behavior"),
		MakeNativeType<Text>("Text", "Graphic", true),
		MakeNativeType<Image>("Image", "Graphic", true),
		MakeNativeType<Graphic>("Graphic", "Behavior"),
		MakeNativeType<Button>("Button", "Selectable", true),
		MakeNativeType<Selectable>("Selectable", "Behavior"),
		MakeNativeType<Behavior>("Behavior", "Component"),
		MakeNativeType<Component>("Component", "Object"),
		MakeNativeType<Material>("Material", "Object"),
		MakeNativeType<AudioClip>("AudioClip", "Object"),
		MakeNativeType<Texture>("Texture2D", "Object"), // C++ の Texture は今 2D しかないので、Unity の名前で出す
		MakeNativeType<Object>("Object", nullptr),
	};
	return types;
}


const NativeType *ou::ScriptBindingsInternal::FindNativeType(int typeId) {
	for (const auto &type : NativeTypes()) {
		if (type.typeId == typeId) { return &type; }
	}
	return nullptr;
}


ScriptBindingsInternal::TypeRegistrar ou::ScriptBindingsInternal::RegisterComponentType(asIScriptEngine *engine, const char *type) {
	const TypeRegistrar c{engine, type};
	c.Method("string get_name() const property", asFUNCTION(GetComponentName));
	c.Method("void set_name(const string &in value) property", asFUNCTION(SetComponentName));
	c.Method("int GetInstanceID() const", asFUNCTION(GetInstanceID));
	c.Method("GameObject@ get_gameObject() const property", asFUNCTION(GetComponentGameObject));
	c.Method("Transform@ get_transform() const property", asFUNCTION(GetComponentTransform));
	c.Method("string get_tag() const property", asFUNCTION(GetTag<Component>));
	c.Method("void set_tag(const string &in value) property", asFUNCTION(SetTag<Component>));
	c.Method("bool CompareTag(const string &in tag) const", asFUNCTION(CompareTag<Component>));
	return c;
}


void ou::ScriptBindingsInternal::RegisterBehaviorMembers(const TypeRegistrar &registrar) {
	registrar.Method("bool get_enabled() const property", asFUNCTION((Get<Behavior, bool, &Behavior::IsEnabled>)));
	registrar.Method("void set_enabled(bool value) property", asFUNCTION((Set<Behavior, bool, &Behavior::SetEnabled>)));
	registrar.Method("bool get_isActiveAndEnabled() const property", asFUNCTION((Get<Behavior, bool, &Behavior::IsActiveAndEnabled>)));
}



// ---- ScriptBindings ----

void *ou::ScriptBindings::CreateObjectHandle(Object *object) {
	return MakeHandle(object);
}


void ou::ScriptBindings::ReleaseObjectHandle(void *handle) {
	if (handle) { Release(static_cast<ObjectHandle *>(handle)); }
}


bool ou::ScriptBindings::SerializeProperty(Serializer &serializer, std::string_view name, int typeId, void *address) {
	switch (typeId) {
	case asTYPEID_BOOL: serializer.Field(name, *static_cast<bool *>(address)); return true;
	case asTYPEID_INT32: serializer.Field(name, *static_cast<int32_t *>(address)); return true;
	case asTYPEID_UINT32: serializer.Field(name, *static_cast<uint32_t *>(address)); return true;
	case asTYPEID_FLOAT: serializer.Field(name, *static_cast<float *>(address)); return true;
	default: break;
	}

	const auto &ids = TypeIds();
	if (typeId == ids.string) { serializer.Field(name, *static_cast<std::string *>(address)); return true; }
	if (typeId == ids.vector2) { serializer.Field(name, *static_cast<Vector2 *>(address)); return true; }
	if (typeId == ids.vector3) { serializer.Field(name, *static_cast<Vector3 *>(address)); return true; }
	if (typeId == ids.quaternion) { serializer.Field(name, *static_cast<Quaternion *>(address)); return true; }
	if (typeId == ids.color) { serializer.Field(name, *static_cast<Color *>(address)); return true; }

	// ハンドルの変数のアドレスは、持ち手のポインタの置き場所
	if (typeId & asTYPEID_OBJHANDLE) {
		const auto baseTypeId = typeId & ~(asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST);
		auto *&handle = *static_cast<ObjectHandle **>(address);
		if (const auto *type = FindNativeType(baseTypeId); type && type->serialize) {
			type->serialize(serializer, name, handle);
			return true;
		}
	}

	// array<T>・enum・ほかのスクリプトのクラス・アセットなどは、まだ保存しない
	return false;
}


void *ou::ScriptBindings::FindComponentOfType(asIScriptEngine *engine, GameObject *gameObject, int typeId) {
	const auto baseTypeId = typeId & ~(asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST);
	const auto *type = engine->GetTypeInfoById(baseTypeId);

	// スクリプトのクラス（Mover など）: MonoBehavior のオブジェクトの中から、その型か派生した型を探す
	if (type && (type->GetFlags() & asOBJ_SCRIPT_OBJECT)) {
		for (auto *behavior : gameObject->GetComponents<MonoBehavior>()) {
			auto *instance = behavior->instance_;
			if (!instance) { continue; }
			const auto *instanceType = instance->GetObjectType();
			if (instanceType == type || instanceType->DerivesFrom(type)) {
				instance->AddRef();
				return instance;
			}
		}
		return nullptr;
	}

	// C++ の Component
	const auto *native = FindNativeType(baseTypeId);
	if (!native || !native->find) {
		ThrowException(std::format("ArgumentException: GetComponent で探せない型です: {}", type ? type->GetName() : "?"));
		return nullptr;
	}
	return MakeHandle(native->find(gameObject));
}


void *ou::ScriptBindings::AddComponentOfType(asIScriptEngine *engine, GameObject *gameObject, int typeId) {
	const auto *type = engine->GetTypeInfoById(typeId);

	// スクリプトのクラス: C++ の MonoBehavior に .as を入れてから付ける（付けたときに Awake が呼ばれるので、先に入れる）
	if (type && (type->GetFlags() & asOBJ_SCRIPT_OBJECT)) {
		auto script = FindMonoScript(type->GetName());
		if (!script) {
			ThrowException(std::format("ArgumentException: スクリプト {}.as が見つかりません。ファイル名とクラス名を一致させてください", type->GetName()));
			return nullptr;
		}
		auto behavior = std::make_unique<MonoBehavior>();
		auto *added = behavior.get();
		added->SetScript(std::move(script));
		gameObject->AddComponentInstance(std::move(behavior));

		if (!added->instance_) { return nullptr; }
		added->instance_->AddRef();
		return added->instance_;
	}

	// C++ の Component: Add Component と同じく TypeRegistry の名前で作る
	const auto *native = FindNativeType(typeId);
	if (!native || !native->canAdd) {
		ThrowException(std::format("ArgumentException: AddComponent で付けられない型です: {}", type ? type->GetName() : "?"));
		return nullptr;
	}
	auto component = TypeRegistry::Create(native->name);
	if (!component) {
		ThrowException(std::format("ArgumentException: {} が TypeRegistry に登録されていません", native->name));
		return nullptr;
	}
	return MakeHandle(gameObject->AddComponentInstance(std::move(component)));
}


// Unity の TryGetComponent(out T)。スクリプトでは gameObject.TryGetComponent(@rb) と書く
// ?&out で受けるので、渡したハンドルの型（typeId）で探す Component を決める
bool ou::ScriptBindings::TryGetComponent(void *out, int typeId, void *selfHandle) {
	if (!(typeId & asTYPEID_OBJHANDLE)) {
		ThrowException("TryGetComponent には、ハンドルの変数を @ を付けて渡してください（例: Rigidbody@ rb; gameObject.TryGetComponent(@rb)）");
		return false;
	}

	auto *gameObject = ResolveOwnerGameObject(static_cast<ObjectHandle *>(selfHandle));
	auto *context = asGetActiveContext();
	if (!gameObject || !context) { return false; }

	auto *found = FindComponentOfType(context->GetEngine(), gameObject, typeId);
	if (!found) { return false; }
	*static_cast<void **>(out) = found;
	return true;
}


// Unity の GetComponent<T>()。T はテンプレート関数の型（generic で登録したので、呼ばれた型を関数から引く）
void ou::ScriptBindings::GetComponentGeneric(asIScriptGeneric *generic) {
	auto *gameObject = ResolveOwnerGameObject(static_cast<ObjectHandle *>(generic->GetObject()));
	void *found = gameObject ? FindComponentOfType(generic->GetEngine(), gameObject, generic->GetFunction()->GetSubTypeId(0)) : nullptr;
	// 参照は FindComponentOfType が 1 つ持たせているので、増やさずに返す
	generic->SetReturnAddress(found);
}


// Unity の AddComponent<T>()
void ou::ScriptBindings::AddComponentGeneric(asIScriptGeneric *generic) {
	auto *gameObject = Resolve<GameObject>(static_cast<ObjectHandle *>(generic->GetObject()));
	void *added = gameObject ? AddComponentOfType(generic->GetEngine(), gameObject, generic->GetFunction()->GetSubTypeId(0)) : nullptr;
	generic->SetReturnAddress(added);
}


void ou::ScriptBindings::RegisterObjectTypes(asIScriptEngine *engine) {
	// 中身はどれも同じ持ち手（ObjectHandle）だが、スクリプトからは別の型に見せる
	auto &types = NativeTypes();
	for (auto &type : types) {
		Check(engine->RegisterObjectType(type.name, 0, asOBJ_REF), type.name);
		type.typeId = engine->GetTypeIdByDecl(type.name);
	}

	// 参照の数え方、親の型への暗黙の変換、親から子への cast<T>
	for (const auto &type : types) {
		const auto ancestors = CollectAncestors(type);
		const TypeRegistrar registrar{engine, type.name};
		registrar.Reference(ancestors);

		const auto declaration = std::format("{}@ opCast()", type.name);
		const auto constDeclaration = std::format("const {}@ opCast() const", type.name);
		for (const auto *ancestor : ancestors) {
			const TypeRegistrar parent{engine, ancestor};
			parent.Method(declaration.c_str(), type.downcast);
			parent.Method(constDeclaration.c_str(), type.downcast);
		}
	}

	// Transform.Translate などの Space::Self / Space::World
	Check(engine->RegisterEnum("Space"), "Space");
	Check(engine->RegisterEnumValue("Space", "World", static_cast<int>(Space::World)), "Space::World");
	Check(engine->RegisterEnumValue("Space", "Self", static_cast<int>(Space::Self)), "Space::Self");
}


void ou::ScriptBindings::RegisterObjectMembers(asIScriptEngine *engine) {
	// ---- Object ----
	{
		const TypeRegistrar o{engine, "Object"};
		o.Method("string get_name() const property", asFUNCTION(GetName));
		o.Method("void set_name(const string &in value) property", asFUNCTION(SetName));
		o.Method("int GetInstanceID() const", asFUNCTION(GetInstanceID));

		engine->SetDefaultNamespace("Object");
		Check(engine->RegisterGlobalFunction("void Destroy(Object@+ obj)", asFUNCTION(Destroy), asCALL_CDECL), "Object::Destroy");
		Check(engine->RegisterGlobalFunction("void DontDestroyOnLoad(Object@+ target)", asFUNCTION(DontDestroyOnLoad), asCALL_CDECL), "Object::DontDestroyOnLoad");
		Check(engine->RegisterGlobalFunction("GameObject@ Instantiate(GameObject@+ original)", asFUNCTION(Instantiate), asCALL_CDECL), "Object::Instantiate");
		Check(engine->RegisterGlobalFunction("GameObject@ Instantiate(GameObject@+ original, Transform@+ parent, bool instantiateInWorldSpace = false)", asFUNCTION(InstantiateWithParent), asCALL_CDECL), "Object::Instantiate");
		Check(engine->RegisterGlobalFunction("GameObject@ Instantiate(GameObject@+ original, const Vector3 &in position, const Quaternion &in rotation, Transform@+ parent = null)", asFUNCTION(InstantiateAt), asCALL_CDECL), "Object::Instantiate");
		engine->SetDefaultNamespace("");
	}

	// ---- GameObject ----
	{
		const TypeRegistrar g{engine, "GameObject"};
		Check(engine->RegisterObjectBehaviour("GameObject", asBEHAVE_FACTORY, "GameObject@ f(const string &in name = \"GameObject\")", asFUNCTION(CreateGameObject), asCALL_CDECL), "GameObject factory");
		g.Method("string get_name() const property", asFUNCTION(GetName));
		g.Method("void set_name(const string &in value) property", asFUNCTION(SetName));
		g.Method("int GetInstanceID() const", asFUNCTION(GetInstanceID));
		g.Method("bool get_activeSelf() const property", asFUNCTION((Get<GameObject, bool, &GameObject::IsActiveSelf>)));
		g.Method("bool get_activeInHierarchy() const property", asFUNCTION((Get<GameObject, bool, &GameObject::IsActiveInHierarchy>)));
		g.Method("void SetActive(bool value)", asFUNCTION((Set<GameObject, bool, &GameObject::SetActive>)));
		g.Method("int get_layer() const property", asFUNCTION((Get<GameObject, int, &GameObject::GetLayer>)));
		g.Method("void set_layer(int value) property", asFUNCTION((Set<GameObject, int, &GameObject::SetLayer>)));
		g.Method("string get_tag() const property", asFUNCTION(GetTag<GameObject>));
		g.Method("void set_tag(const string &in value) property", asFUNCTION(SetTag<GameObject>));
		g.Method("bool CompareTag(const string &in tag) const", asFUNCTION(CompareTag<GameObject>));
		g.Method("Transform@ get_transform() const property", asFUNCTION((GetHandle<GameObject, Transform, &GameObject::GetTransform>)));
	}

	// ---- Component ----
	RegisterComponentType(engine, "Component");

	// ---- Behavior ----
	RegisterBehaviorMembers(RegisterComponentType(engine, "Behavior"));

	// ---- Transform ----
	{
		const auto t = RegisterComponentType(engine, "Transform");
		t.Method("Vector3 get_position() const property", asFUNCTION((Get<Transform, Vector3, &Transform::GetWorldPosition>)));
		t.Method("void set_position(const Vector3 &in value) property", asFUNCTION((Set<Transform, const Vector3 &, &Transform::SetWorldPosition>)));
		t.Method("Vector3 get_localPosition() const property", asFUNCTION((Get<Transform, const Vector3 &, &Transform::GetLocalPosition>)));
		t.Method("void set_localPosition(const Vector3 &in value) property", asFUNCTION((Set<Transform, const Vector3 &, &Transform::SetLocalPosition>)));
		t.Method("Quaternion get_rotation() const property", asFUNCTION((Get<Transform, Quaternion, &Transform::GetWorldRotation>)));
		t.Method("void set_rotation(const Quaternion &in value) property", asFUNCTION((Set<Transform, const Quaternion &, &Transform::SetWorldRotation>)));
		t.Method("Quaternion get_localRotation() const property", asFUNCTION((Get<Transform, const Quaternion &, &Transform::GetLocalRotation>)));
		t.Method("void set_localRotation(const Quaternion &in value) property", asFUNCTION((Set<Transform, const Quaternion &, &Transform::SetLocalRotation>)));
		t.Method("Vector3 get_eulerAngles() const property", asFUNCTION(GetEulerAngles));
		t.Method("void set_eulerAngles(const Vector3 &in value) property", asFUNCTION(SetEulerAngles));
		t.Method("Vector3 get_localEulerAngles() const property", asFUNCTION(GetLocalEulerAngles));
		t.Method("void set_localEulerAngles(const Vector3 &in value) property", asFUNCTION(SetLocalEulerAngles));
		t.Method("Vector3 get_localScale() const property", asFUNCTION((Get<Transform, const Vector3 &, &Transform::GetLocalScale>)));
		t.Method("void set_localScale(const Vector3 &in value) property", asFUNCTION((Set<Transform, const Vector3 &, &Transform::SetLocalScale>)));
		t.Method("Vector3 get_lossyScale() const property", asFUNCTION((Get<Transform, Vector3, &Transform::GetLossyScale>)));
		t.Method("Vector3 get_forward() const property", asFUNCTION((Get<Transform, Vector3, &Transform::GetForward>)));
		t.Method("Vector3 get_right() const property", asFUNCTION(GetRight));
		t.Method("Vector3 get_up() const property", asFUNCTION(GetUp));
		t.Method("Transform@ get_parent() const property", asFUNCTION(GetParent));
		t.Method("void set_parent(Transform@+ value) property", asFUNCTION(SetParent));
		t.Method("int get_childCount() const property", asFUNCTION(GetChildCount));
		t.Method("Transform@ GetChild(int index) const", asFUNCTION(GetChild));
		t.Method("Transform@ Find(const string &in name) const", asFUNCTION(Find));
		t.Method("void Translate(const Vector3 &in translation, Space relativeTo = Space::Self)", asFUNCTION(Translate));
		t.Method("void Translate(float x, float y, float z, Space relativeTo = Space::Self)", asFUNCTION(TranslateXYZ));
		t.Method("void Rotate(const Vector3 &in eulers, Space relativeTo = Space::Self)", asFUNCTION(Rotate));
		t.Method("void Rotate(float xAngle, float yAngle, float zAngle, Space relativeTo = Space::Self)", asFUNCTION(RotateXYZ));
		t.Method("void LookAt(const Vector3 &in worldPosition, const Vector3 &in worldUp = Vector3::up)", asFUNCTION(LookAt));
	}
}


// Unity の GameObject.Find(name)。今のシーンと DontDestroyOnLoad のシーンから、動いている GameObject を名前で探す
void *ou::ScriptBindings::FindGameObject(const std::string &name) {
	// 止めた GameObject の子は探さない（Unity と同じく、動いているものだけ）
	std::function<GameObject *(Transform *)> findIn = [&](Transform *transform) -> GameObject * {
		auto *gameObject = transform->GetGameObject();
		if (!gameObject || !gameObject->IsActiveSelf()) { return nullptr; }
		if (gameObject->GetName() == name) { return gameObject; }
		for (auto *child : transform->GetChildren()) {
			if (auto *found = findIn(child)) { return found; }
		}
		return nullptr;
	};

	for (auto *scene : {SceneManager::GetActiveScene(), SceneManager::GetDontDestroyOnLoadScene()}) {
		if (!scene) { continue; }
		for (auto *root : scene->GetRootGameObjects()) {
			if (auto *found = findIn(root->GetTransform())) { return MakeHandle(found); }
		}
	}
	return nullptr;
}


void ou::ScriptBindings::RegisterGetComponent(asIScriptEngine *engine) {
	// GameObject と Component の型の全部に付ける（Unity と同じく component.GetComponent<T>() でも探せる）
	for (const auto &type : NativeTypes()) {
		if (!type.find && std::string_view(type.name) != "GameObject") { continue; }
		Check(engine->RegisterObjectMethod(type.name, "bool TryGetComponent(?&out component) const", asFUNCTION(TryGetComponent), asCALL_CDECL_OBJLAST), type.name);
		// テンプレート関数は generic でしか登録できない
		Check(engine->RegisterObjectMethod(type.name, "T@ GetComponent<T>() const", asFUNCTION(GetComponentGeneric), asCALL_GENERIC), type.name);
	}
	Check(engine->RegisterObjectMethod("GameObject", "T@ AddComponent<T>()", asFUNCTION(AddComponentGeneric), asCALL_GENERIC), "GameObject::AddComponent");

	engine->SetDefaultNamespace("GameObject");
	Check(engine->RegisterGlobalFunction("GameObject@ Find(const string &in name)", asFUNCTION(FindGameObject), asCALL_CDECL), "GameObject::Find");
	Check(engine->RegisterGlobalFunction("GameObject@ FindWithTag(const string &in tag)", asFUNCTION(FindWithTag), asCALL_CDECL), "GameObject::FindWithTag");
	Check(engine->RegisterGlobalFunction("GameObject@ FindGameObjectWithTag(const string &in tag)", asFUNCTION(FindWithTag), asCALL_CDECL), "GameObject::FindGameObjectWithTag");
	Check(engine->RegisterGlobalFunction("array<GameObject@>@ FindGameObjectsWithTag(const string &in tag)", asFUNCTION(FindGameObjectsWithTag), asCALL_CDECL), "GameObject::FindGameObjectsWithTag");
	engine->SetDefaultNamespace("");

	// 変数の保存で型を見分けるために、値の型の id を覚えておく
	auto &ids = TypeIds();
	ids.string = engine->GetTypeIdByDecl("string");
	ids.vector2 = engine->GetTypeIdByDecl("Vector2");
	ids.vector3 = engine->GetTypeIdByDecl("Vector3");
	ids.quaternion = engine->GetTypeIdByDecl("Quaternion");
	ids.color = engine->GetTypeIdByDecl("Color");
}
