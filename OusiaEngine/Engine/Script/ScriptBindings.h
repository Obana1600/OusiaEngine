#pragma once

#include <string_view>


// 前方宣言
class asIScriptEngine;


namespace ou {

// 前方宣言
class Collision;
class Object;
class Serializer;

// AngelScript に Unity の API（Debug・Vector3・Transform など）を見せる
// ゲームからは使わないので、全部 private にして ScriptEngine だけに見せる
// 静的なものは AngelScript の書き方に合わせて :: で書く（Debug::Log、Vector3::zero、KeyCode::Space）
class ScriptBindings final {
	friend class ScriptEngine;

public: // 静的メンバ関数

	ScriptBindings() = delete;


private: // 静的メンバ関数

	static void Register(asIScriptEngine *engine);

	// ScriptBindings.cpp
	static void RegisterDebug(asIScriptEngine *engine);
	static void RegisterMathf(asIScriptEngine *engine);
	static void RegisterTime(asIScriptEngine *engine);
	static void RegisterInput(asIScriptEngine *engine);
	static void RegisterScreen(asIScriptEngine *engine);

	// ScriptBindingsMath.cpp
	static void RegisterValueTypes(asIScriptEngine *engine);

	// ScriptBindingsObject.cpp
	static void RegisterObjectTypes(asIScriptEngine *engine);
	static void RegisterObjectMembers(asIScriptEngine *engine);
	static void RegisterTryGetComponent(asIScriptEngine *engine);

	// ScriptBindingsPhysics.cpp
	static void RegisterPhysicsTypes(asIScriptEngine *engine);
	static void RegisterPhysicsMembers(asIScriptEngine *engine);

	// C++ のオブジェクトをスクリプトに渡す持ち手を作る（参照の数を 1 増やして返す。nullptr なら nullptr）
	static void *CreateObjectHandle(Object *object);
	static void ReleaseObjectHandle(void *handle);

	// Collision をスクリプトの値の型（Collision）にしたものを作る。使い終わったら DestroyCollision で消す
	static void *CreateCollision(const Collision &collision);
	static void DestroyCollision(void *collision);

	// スクリプトの変数 1 つを保存・読み込み・Inspector に通す（扱えない型なら false）
	static bool SerializeProperty(Serializer &serializer, std::string_view name, int typeId, void *address);

	// AngelScript Language Server（VS Code）が補完に使う as.predefined を、プロジェクトのフォルダに書き出す（エディタだけ）
	static void WritePredefined(asIScriptEngine *engine);

	// Unity の TryGetComponent(out T)（スクリプトの MonoBehavior のオブジェクトも探すので、MonoBehavior の friend）
	static bool TryGetComponent(void *out, int typeId, void *selfHandle);
};

} // namespace ou
