#pragma once

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
#include <vector>

#include <angelscript.h>


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

} // namespace ou::ScriptBindingsInternal
