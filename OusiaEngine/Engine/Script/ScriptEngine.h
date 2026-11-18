#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>


// 前方宣言
class asIScriptEngine;
class asIScriptModule;
struct asSMessageInfo;
class asIScriptContext;
class asIScriptFunction;
class asIScriptObject;
class asITypeInfo;


namespace ou {

// 前方宣言
class MonoBehavior;
class Serializer;
class Collider;
class Collision;


// AngelScript のエンジンと、全ての .as をまとめたモジュール
class ScriptEngine final {
	friend class Engine;
	friend class MonoBehavior;
	friend class ScriptBindings;
	friend class EditorApplication;

public: // エイリアス

	using asUINT = unsigned int;


public: // 静的メンバ関数

	ScriptEngine() = delete;


private: // 静的メンバ変数

	static inline asIScriptEngine *sEngine_ = nullptr;
	static inline asIScriptModule *sModule_ = nullptr;
	static inline std::set<std::pair<int, asUINT>> sSerializeFields_;
	static inline std::map<std::filesystem::path, std::filesystem::file_time_type> sScriptTimes_;


private: // 静的メンバ関数

	static void Initialize();
	static void Finalize();

	static bool Compile();

	static bool IsOutdated();
	static std::map<std::filesystem::path, std::filesystem::file_time_type> FindScriptFiles();

	static asITypeInfo *FindMonoBehaviorClass(const std::string &className);
	static asIScriptObject *CreateObject(asITypeInfo *type, MonoBehavior *native);
	static bool CallMethod(asIScriptObject *object, asIScriptFunction *method, void *argument = nullptr);
	// 引数は setArguments で入れる（Prepare と SetObject のあと、Execute の前に呼ぶ）。object が null なら、メソッドではない関数（delegate など）を呼ぶ
	static bool CallMethodWith(asIScriptObject *object, asIScriptFunction *method, const std::function<void(asIScriptContext *)> &setArguments);
	static void CallCollisionMethod(asIScriptObject *object, asIScriptFunction *method, const Collision &collision);
	static void CallColliderMethod(asIScriptObject *object, asIScriptFunction *method, Collider *collider);
	static void LogException(asIScriptContext *context);

	static void SerializeFields(asIScriptObject *object, Serializer &serializer);
	static bool IsSerializedField(const asITypeInfo *type, asUINT index);
	static void MessageCallback(const asSMessageInfo *message, void *param);
};

} // namespace ou
