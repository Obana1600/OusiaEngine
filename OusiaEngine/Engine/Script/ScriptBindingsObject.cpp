#include "ScriptBindings.h"
#include "Core/Object.h"
#include "Math/Color.h"
#include "Math/Mathf.h"
#include "Math/Quaternion.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Physics/CharacterController.h"
#include "Physics/Collider.h"
#include "Physics/Rigidbody.h"
#include "Scene/Behavior.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"
#include "Script/MonoBehavior.h"
#include "Script/ScriptBindingsInternal.h"
#include "Serialize/Serializer.h"

#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>

#include <angelscript.h>

using namespace ou;
using namespace ou::ScriptBindingsInternal;


namespace {

// ---- 持ち手 ----

// 同じオブジェクトには同じ持ち手を返す（スクリプトの a is b が成り立つように）
std::unordered_map<uint32_t, ObjectHandle *> &Handles() {
	static std::unordered_map<uint32_t, ObjectHandle *> handles;
	return handles;
}

const char *DescribeType(const Object *object) {
	if (dynamic_cast<const GameObject *>(object)) { return "GameObject"; }
	if (dynamic_cast<const Transform *>(object)) { return "Transform"; }
	if (dynamic_cast<const CharacterController *>(object)) { return "CharacterController"; }
	if (dynamic_cast<const Collider *>(object)) { return "Collider"; }
	if (dynamic_cast<const Rigidbody *>(object)) { return "Rigidbody"; }
	if (dynamic_cast<const Behavior *>(object)) { return "Behavior"; }
	if (dynamic_cast<const Component *>(object)) { return "Component"; }
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


// ---- 型の id（変数の保存と TryGetComponent で型を見分けるのに使う。登録したあとに引いて覚えておく） ----

struct ScriptTypeIds {
	int string = 0;
	int vector2 = 0;
	int vector3 = 0;
	int quaternion = 0;
	int color = 0;
	int gameObject = 0;
	int component = 0;
	int behavior = 0;
	int transform = 0;
	int collider = 0;
	int rigidbody = 0;
	int characterController = 0;
};

ScriptTypeIds &TypeIds() {
	static ScriptTypeIds ids;
	return ids;
}

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

// TryGetComponent で、C++ の Component の型を見分ける
template <typename T>
Component *FindComponent(GameObject *gameObject) {
	return gameObject->GetComponent<T>();
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

	auto *handle = new ObjectHandle{1, object->GetInstanceId(), DescribeType(object)};
	handles.emplace(handle->instanceId, handle);
	return handle;
}


void ou::ScriptBindingsInternal::Release(ObjectHandle *self) {
	if (--self->refCount > 0) { return; }
	Handles().erase(self->instanceId);
	delete self;
}


ScriptBindingsInternal::TypeRegistrar ou::ScriptBindingsInternal::RegisterComponentType(
	asIScriptEngine *engine, const char *type, std::initializer_list<const char *> baseTypes) {
	const TypeRegistrar c{engine, type};
	c.Reference(baseTypes);
	c.Method("string get_name() const property", asFUNCTION(GetComponentName));
	c.Method("void set_name(const string &in value) property", asFUNCTION(SetComponentName));
	c.Method("int GetInstanceID() const", asFUNCTION(GetInstanceID));
	c.Method("GameObject@ get_gameObject() const property", asFUNCTION(GetComponentGameObject));
	c.Method("Transform@ get_transform() const property", asFUNCTION(GetComponentTransform));
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
		if (baseTypeId == ids.gameObject) { SerializeHandle<GameObject>(serializer, name, handle); return true; }
		if (baseTypeId == ids.transform) { SerializeHandle<Transform>(serializer, name, handle); return true; }
		if (baseTypeId == ids.characterController) { SerializeHandle<CharacterController>(serializer, name, handle); return true; }
		if (baseTypeId == ids.collider) { SerializeHandle<Collider>(serializer, name, handle); return true; }
		if (baseTypeId == ids.rigidbody) { SerializeHandle<Rigidbody>(serializer, name, handle); return true; }
		if (baseTypeId == ids.behavior) { SerializeHandle<Behavior>(serializer, name, handle); return true; }
		if (baseTypeId == ids.component) { SerializeHandle<Component>(serializer, name, handle); return true; }
	}

	// array<T>・enum・ほかのスクリプトのクラスなどは、まだ保存しない
	return false;
}


// Unity の TryGetComponent(out T)。スクリプトでは gameObject.TryGetComponent(@rb) と書く
// ?&out で受けるので、渡したハンドルの型（typeId）で探す Component を決める
bool ou::ScriptBindings::TryGetComponent(void *out, int typeId, void *selfHandle) {
	if (!(typeId & asTYPEID_OBJHANDLE)) {
		ThrowException("TryGetComponent には、ハンドルの変数を @ を付けて渡してください（例: Rigidbody@ rb; gameObject.TryGetComponent(@rb)）");
		return false;
	}

	// 自分は GameObject か Component
	auto *self = Resolve<Object>(static_cast<ObjectHandle *>(selfHandle));
	if (!self) { return false; }
	auto *gameObject = dynamic_cast<GameObject *>(self);
	if (!gameObject) {
		if (auto *component = dynamic_cast<Component *>(self)) { gameObject = component->GetGameObject(); }
	}
	if (!gameObject) { return false; }

	const auto baseTypeId = typeId & ~(asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST);

	// スクリプトのクラス（Mover@ など）: MonoBehavior のオブジェクトの中から、その型か派生した型を探す
	auto *engine = asGetActiveContext() ? asGetActiveContext()->GetEngine() : nullptr;
	const auto *type = engine ? engine->GetTypeInfoById(baseTypeId) : nullptr;
	if (type && (type->GetFlags() & asOBJ_SCRIPT_OBJECT)) {
		for (auto *behavior : gameObject->GetComponents<MonoBehavior>()) {
			auto *instance = behavior->instance_;
			if (!instance) { continue; }
			const auto *instanceType = instance->GetObjectType();
			if (instanceType == type || instanceType->DerivesFrom(type)) {
				instance->AddRef();
				*static_cast<asIScriptObject **>(out) = instance;
				return true;
			}
		}
		return false;
	}

	// C++ の Component
	const auto &ids = TypeIds();
	Component *found = nullptr;
	if (baseTypeId == ids.transform) { found = FindComponent<Transform>(gameObject); }
	else if (baseTypeId == ids.rigidbody) { found = FindComponent<Rigidbody>(gameObject); }
	else if (baseTypeId == ids.characterController) { found = FindComponent<CharacterController>(gameObject); }
	else if (baseTypeId == ids.collider) { found = FindComponent<Collider>(gameObject); }
	else if (baseTypeId == ids.behavior) { found = FindComponent<Behavior>(gameObject); }
	else if (baseTypeId == ids.component) { found = FindComponent<Component>(gameObject); }
	else {
		ThrowException(std::format("TryGetComponent で探せない型です: {}", engine && type ? type->GetName() : "?"));
		return false;
	}

	if (!found) { return false; }
	*static_cast<ObjectHandle **>(out) = MakeHandle(found);
	return true;
}


void ou::ScriptBindings::RegisterObjectTypes(asIScriptEngine *engine) {
	// 中身はどれも同じ持ち手（ObjectHandle）だが、スクリプトからは別の型に見せる
	for (const auto *type : {"Object", "GameObject", "Component", "Behavior", "Transform", "Collider", "Rigidbody", "CharacterController"}) {
		Check(engine->RegisterObjectType(type, 0, asOBJ_REF), type);
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
		o.Reference({});
		o.Method("string get_name() const property", asFUNCTION(GetName));
		o.Method("void set_name(const string &in value) property", asFUNCTION(SetName));
		o.Method("int GetInstanceID() const", asFUNCTION(GetInstanceID));
		o.Downcast<GameObject>("GameObject");
		o.Downcast<Component>("Component");
		o.Downcast<Behavior>("Behavior");
		o.Downcast<Transform>("Transform");
		o.Downcast<Collider>("Collider");
		o.Downcast<Rigidbody>("Rigidbody");
		o.Downcast<CharacterController>("CharacterController");

		engine->SetDefaultNamespace("Object");
		Check(engine->RegisterGlobalFunction("void Destroy(Object@+ obj)", asFUNCTION(Destroy), asCALL_CDECL), "Object::Destroy");
		engine->SetDefaultNamespace("");
	}

	// ---- GameObject ----
	{
		const TypeRegistrar g{engine, "GameObject"};
		g.Reference({"Object"});
		Check(engine->RegisterObjectBehaviour("GameObject", asBEHAVE_FACTORY, "GameObject@ f(const string &in name = \"GameObject\")", asFUNCTION(CreateGameObject), asCALL_CDECL), "GameObject factory");
		g.Method("string get_name() const property", asFUNCTION(GetName));
		g.Method("void set_name(const string &in value) property", asFUNCTION(SetName));
		g.Method("int GetInstanceID() const", asFUNCTION(GetInstanceID));
		g.Method("bool get_activeSelf() const property", asFUNCTION((Get<GameObject, bool, &GameObject::IsActiveSelf>)));
		g.Method("bool get_activeInHierarchy() const property", asFUNCTION((Get<GameObject, bool, &GameObject::IsActiveInHierarchy>)));
		g.Method("void SetActive(bool value)", asFUNCTION((Set<GameObject, bool, &GameObject::SetActive>)));
		g.Method("int get_layer() const property", asFUNCTION((Get<GameObject, int, &GameObject::GetLayer>)));
		g.Method("void set_layer(int value) property", asFUNCTION((Set<GameObject, int, &GameObject::SetLayer>)));
		g.Method("Transform@ get_transform() const property", asFUNCTION((GetHandle<GameObject, Transform, &GameObject::GetTransform>)));
	}

	// ---- Component ----
	{
		const auto c = RegisterComponentType(engine, "Component", {"Object"});
		c.Downcast<Behavior>("Behavior");
		c.Downcast<Transform>("Transform");
		c.Downcast<Collider>("Collider");
		c.Downcast<Rigidbody>("Rigidbody");
		c.Downcast<CharacterController>("CharacterController");
	}

	// ---- Behavior ----
	{
		const auto b = RegisterComponentType(engine, "Behavior", {"Component", "Object"});
		RegisterBehaviorMembers(b);
		b.Downcast<Collider>("Collider");
		b.Downcast<CharacterController>("CharacterController");
	}

	// ---- Transform ----
	{
		const auto t = RegisterComponentType(engine, "Transform", {"Component", "Object"});
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


void ou::ScriptBindings::RegisterTryGetComponent(asIScriptEngine *engine) {
	for (const auto *type : {"GameObject", "Component", "Behavior", "Transform", "Collider", "Rigidbody", "CharacterController"}) {
		Check(engine->RegisterObjectMethod(type, "bool TryGetComponent(?&out component) const", asFUNCTION(TryGetComponent), asCALL_CDECL_OBJLAST), type);
	}

	// 変数の保存と TryGetComponent で型を見分けるために、型の id を覚えておく
	auto &ids = TypeIds();
	ids.string = engine->GetTypeIdByDecl("string");
	ids.vector2 = engine->GetTypeIdByDecl("Vector2");
	ids.vector3 = engine->GetTypeIdByDecl("Vector3");
	ids.quaternion = engine->GetTypeIdByDecl("Quaternion");
	ids.color = engine->GetTypeIdByDecl("Color");
	ids.gameObject = engine->GetTypeIdByDecl("GameObject");
	ids.component = engine->GetTypeIdByDecl("Component");
	ids.behavior = engine->GetTypeIdByDecl("Behavior");
	ids.transform = engine->GetTypeIdByDecl("Transform");
	ids.collider = engine->GetTypeIdByDecl("Collider");
	ids.rigidbody = engine->GetTypeIdByDecl("Rigidbody");
	ids.characterController = engine->GetTypeIdByDecl("CharacterController");
}
