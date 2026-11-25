#pragma once

#include "Asset/AssetDatabase.h"
#include "Core/Object.h"
#include "Diagnostics/Log.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"

#include <cstdint>
#include <format>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <angelscript.h>
#include <magic_enum/magic_enum.hpp>
#include <scriptarray/scriptarray.h>


namespace ou {
class Component;
class GameObject;
class Serializer;
} // namespace ou


// ScriptBindings*.cpp の中だけで使う
namespace ou::ScriptBindingsInternal {

// 登録の失敗はエンジンの書き間違いなので、宣言をそのままログに出す
inline void Check(int result, std::string_view declaration) {
	if (result < 0) {
		Log::Error("AngelScript への登録に失敗しました ({}): {}", result, declaration);
	}
}

// スクリプトに例外を投げる（C++ の関数から戻ったところでスクリプトが止まり、ScriptEngine がログに出す）
inline void ThrowException(const std::string &message) {
	if (auto *context = asGetActiveContext()) {
		context->SetException(message.c_str());
	} else {
		Log::Error("{}", message);
	}
}

// forward を +Z、upwards をおおよその +Y に向ける回転（Unity の Quaternion.LookRotation。ScriptBindingsMath.cpp）
Quaternion LookRotation(const Vector3 &forward, const Vector3 &upwards);


// ---- 持ち手（Unity の「偽の null」の代わり。ScriptBindingsObject.cpp） ----

// C++ のオブジェクトはシーンが消すので、スクリプトはポインタではなく instanceId を持ち、触るたびに引き直す
// 消えたあとに触ると、exe が落ちる代わりに MissingReferenceException になる
struct ObjectHandle {
	int refCount = 1;
	uint32_t instanceId = 0;
	const char *typeName = "Object"; // 消えたあとの例外の文面に使う
	std::shared_ptr<Object> asset;	 // アセット（プレハブ・Material など）なら、スクリプトが持っているあいだ手放さない
};

// 同じオブジェクトには同じ持ち手を返す（参照の数を 1 増やして返す。nullptr なら nullptr）
ObjectHandle *MakeHandle(Object *object);

inline void AddRef(ObjectHandle *self) {
	self->refCount++;
}

void Release(ObjectHandle *self);

// 持ち手からオブジェクトを引く。消えていたら Unity と同じ MissingReferenceException にする
template <typename T>
T *Resolve(ObjectHandle *self) {
	auto *object = dynamic_cast<T *>(Object::FindObjectFromInstanceId(self->instanceId));
	if (!object) {
		ThrowException(std::format("MissingReferenceException: この {} は破棄されています。破棄したものを覚えておかないか、使う前に確かめてください", self->typeName));
	}
	return object;
}

// 親の型へは暗黙に変えられる（同じ持ち手を返す）
inline ObjectHandle *Upcast(ObjectHandle *self) {
	AddRef(self);
	return self;
}

// 子の型へは cast<Transform>(component) で変える。違う型なら null
template <typename T>
ObjectHandle *Downcast(ObjectHandle *self) {
	if (!dynamic_cast<T *>(Object::FindObjectFromInstanceId(self->instanceId))) { return nullptr; }
	AddRef(self);
	return self;
}


// ---- 読み書きの手伝い（関数の最後の引数が自分。asCALL_CDECL_OBJLAST） ----

template <typename T, typename R, R (T::*Getter)() const>
std::decay_t<R> Get(ObjectHandle *self) {
	auto *target = Resolve<T>(self);
	return target ? (target->*Getter)() : std::decay_t<R>{};
}

template <typename T, typename V, void (T::*Setter)(V)>
void Set(V value, ObjectHandle *self) {
	if (auto *target = Resolve<T>(self)) { (target->*Setter)(value); }
}

// C++ のポインタを返す関数は、持ち手にして返す
template <typename T, typename R, R *(T::*Getter)() const>
ObjectHandle *GetHandle(ObjectHandle *self) {
	auto *target = Resolve<T>(self);
	return target ? MakeHandle((target->*Getter)()) : nullptr;
}

// 引数のない関数を呼ぶ（Play・Stop など）
template <typename T, void (T::*Function)()>
void Call(ObjectHandle *self) {
	if (auto *target = Resolve<T>(self)) { (target->*Function)(); }
}

// スクリプトの enum は 32 ビットの int なので、int で受け渡す
// C++ の enum にない値なら例外にして false（C++ の enum は uint8_t のものもあるので、先に範囲を見る）
template <typename E>
bool CheckEnumValue(int value) {
	using Underlying = std::underlying_type_t<E>;
	if (std::in_range<Underlying>(value) && magic_enum::enum_contains<E>(static_cast<Underlying>(value))) { return true; }
	ThrowException(std::format("ArgumentException: {} にない値です: {}", magic_enum::enum_type_name<E>(), value));
	return false;
}

template <typename T, typename E, E (T::*Getter)() const>
int GetEnum(ObjectHandle *self) {
	auto *target = Resolve<T>(self);
	return target ? static_cast<int>((target->*Getter)()) : 0;
}

template <typename T, typename E, void (T::*Setter)(E)>
void SetEnum(int value, ObjectHandle *self) {
	if (!CheckEnumValue<E>(value)) { return; }
	if (auto *target = Resolve<T>(self)) { (target->*Setter)(static_cast<E>(value)); }
}

// C++ の enum の名前と値を、そのままスクリプトの enum にする（ForceMode::Impulse のように書く）
template <typename E>
void RegisterEnum(asIScriptEngine *engine, const char *name) {
	Check(engine->RegisterEnum(name), name);
	for (const auto &[value, valueName] : magic_enum::enum_entries<E>()) {
		const std::string text(valueName);
		Check(engine->RegisterEnumValue(name, text.c_str(), static_cast<int>(value)), text);
	}
}


// ---- 配列（Unity の T[]。array<T@> で返す） ----

// objects の持ち手を並べた配列を作る。declaration は "array<Collider@>" など
template <typename T>
CScriptArray *MakeHandleArray(const std::vector<T *> &objects, const char *declaration) {
	auto *context = asGetActiveContext();
	if (!context) { return nullptr; }
	auto *type = context->GetEngine()->GetTypeInfoByDecl(declaration);
	auto *array = CScriptArray::Create(type, static_cast<asUINT>(objects.size()));
	for (size_t i = 0; i < objects.size(); i++) {
		// MakeHandle が持たせた参照を、そのまま配列に持たせる
		*static_cast<void **>(array->At(static_cast<asUINT>(i))) = MakeHandle(objects[i]);
	}
	return array;
}

// 値の型の配列を作る。declaration は "array<Vector3>" など
template <typename T>
CScriptArray *MakeValueArray(const std::vector<T> &values, const char *declaration) {
	auto *context = asGetActiveContext();
	if (!context) { return nullptr; }
	auto *type = context->GetEngine()->GetTypeInfoByDecl(declaration);
	auto *array = CScriptArray::Create(type, static_cast<asUINT>(values.size()));
	for (size_t i = 0; i < values.size(); i++) {
		*static_cast<T *>(array->At(static_cast<asUINT>(i))) = values[i];
	}
	return array;
}


// ---- アセット（C++ は shared_ptr で持つ） ----

// 持ち手はポインタしか引けないので、アセットのデータベースから shared_ptr を引き直す
// アセットでないもの（Renderer が複製した Material など）は nullptr
template <typename T>
std::shared_ptr<T> FindSharedAsset(T *asset) {
	if (!asset) { return nullptr; }
	const auto reference = AssetDatabase::FindAssetReference(asset);
	if (!reference) { return nullptr; }
	return std::dynamic_pointer_cast<T>(AssetDatabase::LoadAsset(reference->guid, reference->subAsset));
}

// スクリプトで作ったもの（Sprite.Create・new Material など）の持ち手。スクリプトが持っているあいだ、持ち手が shared_ptr を持つ
inline ObjectHandle *MakeOwningHandle(const std::shared_ptr<Object> &object) {
	auto *handle = MakeHandle(object.get());
	if (handle && !handle->asset) { handle->asset = object; }
	return handle;
}

// null を渡せば外す（out は nullptr で true）。アセットでもスクリプトで作ったものでもないもの（renderer.material の複製など）は渡せない
template <typename T>
bool ResolveSharedAsset(ObjectHandle *handle, std::shared_ptr<T> &out) {
	out = nullptr;
	if (!handle) { return true; }

	auto *asset = Resolve<T>(handle);
	if (!asset) { return false; }
	out = handle->asset.get() == asset ? std::dynamic_pointer_cast<T>(handle->asset) : FindSharedAsset(asset);
	if (!out) {
		ThrowException("ArgumentException: アセットとして保存されていないものは渡せません");
		return false;
	}
	return true;
}

// shared_ptr のアセットを返す関数は、持ち手にして返す
template <typename T, typename A, const std::shared_ptr<A> &(T::*Getter)() const>
ObjectHandle *GetAssetHandle(ObjectHandle *self) {
	auto *target = Resolve<T>(self);
	return target ? MakeHandle((target->*Getter)().get()) : nullptr;
}

// shared_ptr のアセットを受け取る関数に、アセットの持ち手を渡す（null なら外す）
template <typename T, typename A, void (T::*Setter)(std::shared_ptr<A>)>
void SetAssetHandle(ObjectHandle *asset, ObjectHandle *self) {
	auto *target = Resolve<T>(self);
	if (!target) { return; }

	std::shared_ptr<A> shared;
	if (ResolveSharedAsset(asset, shared)) { (target->*Setter)(std::move(shared)); }
}


// ---- 参照の型の登録の手伝い ----

struct TypeRegistrar {
	asIScriptEngine *engine;
	const char *type;

	void Method(const char *declaration, const asSFuncPtr &function) const {
		Check(engine->RegisterObjectMethod(type, declaration, function, asCALL_CDECL_OBJLAST), declaration);
	}

	void Behaviour(asEBehaviours behaviour, const char *declaration, const asSFuncPtr &function) const {
		Check(engine->RegisterObjectBehaviour(type, behaviour, declaration, function, asCALL_CDECL_OBJLAST), declaration);
	}

	// 参照の数え方と、親の型への暗黙の変換
	void Reference(const std::vector<const char *> &baseTypes) const {
		Behaviour(asBEHAVE_ADDREF, "void f()", asFUNCTION(AddRef));
		Behaviour(asBEHAVE_RELEASE, "void f()", asFUNCTION(Release));
		for (const auto *baseType : baseTypes) {
			const auto declaration = std::format("{}@ opImplCast()", baseType);
			Method(declaration.c_str(), asFUNCTION(Upcast));
			const auto constDeclaration = std::format("const {}@ opImplCast() const", baseType);
			Method(constDeclaration.c_str(), asFUNCTION(Upcast));
		}
	}

	template <typename T>
	void Downcast(const char *derivedType) const {
		const auto declaration = std::format("{}@ opCast()", derivedType);
		Method(declaration.c_str(), asFUNCTION(ScriptBindingsInternal::Downcast<T>));
		const auto constDeclaration = std::format("const {}@ opCast() const", derivedType);
		Method(constDeclaration.c_str(), asFUNCTION(ScriptBindingsInternal::Downcast<T>));
	}
};

// ---- スクリプトに見せる C++ の型の表（ScriptBindingsObject.cpp） ----

// Object・GameObject・Component の派生など、持ち手で渡す型
// 型の登録・親への変換（opImplCast）・子への変換（opCast）・GetComponent<T>・変数の保存は、この表から作る
struct NativeType {
	const char *name;
	const char *parent; // Object だけ nullptr
	bool (*isInstance)(const Object *object);
	asSFuncPtr downcast;												  // Downcast<T>
	Component *(*find)(const GameObject *gameObject);					  // Component でなければ nullptr
	void (*serialize)(Serializer &, std::string_view, ObjectHandle *&); // 保存できなければ nullptr
	bool canAdd;														  // AddComponent<T> で作れる（名前は TypeRegistry と同じ）
	int typeId = 0;														  // 登録したあとに引く
};

// 子の型が親より先に並ぶ（持ち手の型の名前と GetComponent<T> は前から見る）
std::vector<NativeType> &NativeTypes();

// スクリプトの型の id から表の行を引く（なければ nullptr）
const NativeType *FindNativeType(int typeId);

// Component の型に name・gameObject・transform を足す（型の登録と親子の変換は RegisterObjectTypes が表から行う）
TypeRegistrar RegisterComponentType(asIScriptEngine *engine, const char *type);

// Behavior の enabled などを足す（ScriptBindingsObject.cpp）
void RegisterBehaviorMembers(const TypeRegistrar &registrar);

// アセットの型（Object の派生で Component でないもの）に name・GetInstanceID を足す（ScriptBindingsComponents.cpp）
TypeRegistrar RegisterAssetType(asIScriptEngine *engine, const char *type);

// Renderer の enabled・material などを足す（ScriptBindingsComponents.cpp。Renderer の派生のすべてに付ける）
void RegisterRendererMembers(const TypeRegistrar &registrar);

} // namespace ou::ScriptBindingsInternal
