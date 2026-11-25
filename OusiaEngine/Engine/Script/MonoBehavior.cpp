#include "MonoBehavior.h"
#include "Core/Application.h"
// #include "Physics/Collider.h"
// #include "Physics/Collision.h"
#include "Script/MonoScript.h"
#include "Script/ScriptEngine.h"
#include "Diagnostics/Log.h"
#include "Serialize/Serializer.h"

#include <algorithm>
#include <array>
#include <utility>

#include <angelscript.h>

using namespace ou;


namespace {

// エンジンが呼ぶメッセージは、Inspector の一覧に出さない（C# では private にするのが普通なので、Unity の一覧にも出ない）
constexpr std::array<std::string_view, 19> kMessageNames = {
	"Awake", "Start", "FixedUpdate", "Update", "LateUpdate", "OnDestroy", "OnApplicationQuit", "OnEnable", "OnDisable", "OnValidate",
	"OnDrawGizmos", "OnDrawGizmosSelected",
	"OnCollisionEnter", "OnCollisionStay", "OnCollisionExit", "OnTriggerEnter", "OnTriggerStay", "OnTriggerExit", "OnControllerColliderHit"};

// 引数の型から、渡す値の型を決める（使えない型なら false）
bool ToListenerMode(asIScriptEngine *engine, int typeId, PersistentListenerMode &mode) {
	switch (typeId) {
	case asTYPEID_INT32: mode = PersistentListenerMode::Int; return true;
	case asTYPEID_FLOAT: mode = PersistentListenerMode::Float; return true;
	case asTYPEID_BOOL: mode = PersistentListenerMode::Bool; return true;
	default: break;
	}
	if (typeId == engine->GetTypeIdByDecl("string")) {
		mode = PersistentListenerMode::String;
		return true;
	}
	return false;
}

// void で、引数が 0 か 1 つ（int・float・bool・string）の、外から呼べるメソッドなら、渡す値の型を返す
bool FindListenerMode(asIScriptEngine *engine, const asIScriptFunction *method, PersistentListenerMode &mode) {
	if (method->IsPrivate() || method->IsProtected() || method->GetReturnTypeId() != asTYPEID_VOID) { return false; }
	if (std::find(kMessageNames.begin(), kMessageNames.end(), method->GetName()) != kMessageNames.end()) { return false; }

	if (method->GetParamCount() == 0) {
		mode = PersistentListenerMode::Void;
		return true;
	}
	if (method->GetParamCount() != 1) { return false; }

	int typeId = 0;
	asDWORD flags = 0;
	method->GetParam(0, &typeId, &flags);
	if (flags & (asTM_OUTREF | asTM_INOUTREF)) { return false; }
	return ToListenerMode(engine, typeId, mode);
}

} // namespace


ou::MonoBehavior::~MonoBehavior() {
	ReleaseInstance();
}


void ou::MonoBehavior::Awake() {
	if (!Application::IsPlaying()) { return; }
	isAwakeCalled_ = true;
	CallMessage(methods_.awake);
}


void ou::MonoBehavior::Start() {
	CallMessage(methods_.start);
}


void ou::MonoBehavior::FixedUpdate() {
	CallMessage(methods_.fixedUpdate);
}


void ou::MonoBehavior::Update() {
	CallMessage(methods_.update);
}


void ou::MonoBehavior::LateUpdate() {
	CallMessage(methods_.lateUpdate);
}


void ou::MonoBehavior::OnDestroy() {
	CallMessage(methods_.onDestroy);
}


void ou::MonoBehavior::OnApplicationQuit() {
	CallMessage(methods_.onApplicationQuit);
}


void ou::MonoBehavior::OnEnable() {
	CallMessage(methods_.onEnable);
}


void ou::MonoBehavior::OnDisable() {
	CallMessage(methods_.onDisable);
}


void ou::MonoBehavior::OnCollisionEnter(const Collision &collision) {
	CallCollisionMessage(methods_.onCollisionEnter, collision);
}


void ou::MonoBehavior::OnCollisionStay(const Collision &collision) {
	CallCollisionMessage(methods_.onCollisionStay, collision);
}


void ou::MonoBehavior::OnCollisionExit(const Collision &collision) {
	CallCollisionMessage(methods_.onCollisionExit, collision);
}


void ou::MonoBehavior::OnTriggerEnter(Collider *other) {
	CallTriggerMessage(methods_.onTriggerEnter, other);
}


void ou::MonoBehavior::OnTriggerStay(Collider *other) {
	CallTriggerMessage(methods_.onTriggerStay, other);
}


void ou::MonoBehavior::OnTriggerExit(Collider *other) {
	CallTriggerMessage(methods_.onTriggerExit, other);
}


void ou::MonoBehavior::OnControllerColliderHit(const ControllerColliderHit &hit) {
	if (!methods_.onControllerColliderHit || !isAwakeCalled_) { return; }
	ScriptEngine::CallControllerColliderHitMethod(instance_, methods_.onControllerColliderHit, hit);
}


void ou::MonoBehavior::OnValidate() {
	// Unity と同じく、編集中（Awake 前）でも呼ぶので CallMessage は使わない
	if (!instance_ || !methods_.onValidate) { return; }
	ScriptEngine::CallMethod(instance_, methods_.onValidate);
}


void ou::MonoBehavior::OnDrawGizmos() {
	// OnValidate と同じく、編集中（Awake 前）でも呼ぶ
	if (!instance_ || !methods_.onDrawGizmos) { return; }
	ScriptEngine::CallMethod(instance_, methods_.onDrawGizmos);
}


void ou::MonoBehavior::OnDrawGizmosSelected() {
	if (!instance_ || !methods_.onDrawGizmosSelected) { return; }
	ScriptEngine::CallMethod(instance_, methods_.onDrawGizmosSelected);
}


void ou::MonoBehavior::Serialize(Serializer &serializer) {
	Behavior::Serialize(serializer);

	// スクリプトで後ろのフィールドの並びが決まるので、ここだけは読んだその場で付け替える（Unity の MonoBehaviour も同じ）
	// 書くときは script が変わらないので、何もしない
	auto script = script_;
	serializer.Asset("script", script);
	if (script != script_) {
		SetScript(std::move(script));
	}

	if (const auto *json = serializer.FindReadingJson()) {
		savedJson_.update(*json);
	}

	if (instance_) {
		ScriptEngine::SerializeFields(instance_, serializer);
		return;
	}

	if (auto *json = serializer.FindWritingJson(); json && savedJson_.is_object()) {
		for (const auto &[key, value] : savedJson_.items()) {
			if (!json->contains(key)) { (*json)[key] = value; }
		}
	}
}


void ou::MonoBehavior::SetScript(std::shared_ptr<MonoScript> script) {
	script_ = std::move(script);
	CreateInstance();
}


void ou::MonoBehavior::CreateInstance() {
	ReleaseInstance();
	if (!script_) { return; }

	auto *type = ScriptEngine::FindMonoBehaviorClass(script_->GetName());
	if (!type) { return; }

	instance_ = ScriptEngine::CreateObject(type, this);
	if (!instance_) { return; }

	methods_.awake = type->GetMethodByDecl("void Awake()");
	methods_.start = type->GetMethodByDecl("void Start()");
	methods_.fixedUpdate = type->GetMethodByDecl("void FixedUpdate()");
	methods_.update = type->GetMethodByDecl("void Update()");
	methods_.lateUpdate = type->GetMethodByDecl("void LateUpdate()");
	methods_.onDestroy = type->GetMethodByDecl("void OnDestroy()");
	methods_.onApplicationQuit = type->GetMethodByDecl("void OnApplicationQuit()");
	methods_.onEnable = type->GetMethodByDecl("void OnEnable()");
	methods_.onDisable = type->GetMethodByDecl("void OnDisable()");
	methods_.onValidate = type->GetMethodByDecl("void OnValidate()");
	methods_.onDrawGizmos = type->GetMethodByDecl("void OnDrawGizmos()");
	methods_.onDrawGizmosSelected = type->GetMethodByDecl("void OnDrawGizmosSelected()");
	methods_.onCollisionEnter = type->GetMethodByDecl("void OnCollisionEnter(Collision)");
	methods_.onCollisionStay = type->GetMethodByDecl("void OnCollisionStay(Collision)");
	methods_.onCollisionExit = type->GetMethodByDecl("void OnCollisionExit(Collision)");
	methods_.onTriggerEnter = type->GetMethodByDecl("void OnTriggerEnter(Collider@)");
	methods_.onTriggerStay = type->GetMethodByDecl("void OnTriggerStay(Collider@)");
	methods_.onTriggerExit = type->GetMethodByDecl("void OnTriggerExit(Collider@)");
	methods_.onControllerColliderHit = type->GetMethodByDecl("void OnControllerColliderHit(ControllerColliderHit)");
}


void ou::MonoBehavior::ReleaseInstance() {
	methods_ = {};
	if (instance_) {
		instance_->Release();
		instance_ = nullptr;
	}
}


void ou::MonoBehavior::CallMessage(asIScriptFunction *method) {
	if (!method || !isAwakeCalled_) { return; }
	ScriptEngine::CallMethod(instance_, method);
}


void ou::MonoBehavior::CallCollisionMessage(asIScriptFunction *method, const Collision &collision) {
	if (!method || !isAwakeCalled_) { return; }
	ScriptEngine::CallCollisionMethod(instance_, method, collision);
}


void ou::MonoBehavior::CallTriggerMessage(asIScriptFunction *method, Collider *other) {
	if (!method || !isAwakeCalled_) { return; }
	ScriptEngine::CallColliderMethod(instance_, method, other);
}


void ou::MonoBehavior::InvokeListenerMethod(std::string_view name, PersistentListenerMode mode, int32_t intArgument, float floatArgument, const std::string &stringArgument, bool boolArgument) {
	if (!instance_) { return; }

	auto *engine = ScriptEngine::sEngine_;
	const auto *type = instance_->GetObjectType();
	for (asUINT i = 0; i < type->GetMethodCount(); i++) {
		auto *method = type->GetMethodByIndex(i);
		PersistentListenerMode methodMode{};
		if (name != method->GetName() || !FindListenerMode(engine, method, methodMode) || methodMode != mode) { continue; }

		ScriptEngine::CallMethodWith(instance_, method, [&](asIScriptContext *context) {
			int typeId = 0;
			asDWORD flags = 0;
			if (method->GetParamCount() == 1) { method->GetParam(0, &typeId, &flags); }

			switch (mode) {
			case PersistentListenerMode::Void: break;
			case PersistentListenerMode::Int: context->SetArgDWord(0, static_cast<asDWORD>(intArgument)); break;
			case PersistentListenerMode::Float: context->SetArgFloat(0, floatArgument); break;
			case PersistentListenerMode::Bool: context->SetArgByte(0, boolArgument ? 1 : 0); break;
			case PersistentListenerMode::String:
				// const string &in は参照、string は値で渡す（値のときは AngelScript が複製する）
				if (flags & asTM_INREF) {
					context->SetArgAddress(0, const_cast<std::string *>(&stringArgument));
				} else {
					context->SetArgObject(0, const_cast<std::string *>(&stringArgument));
				}
				break;
			}
		});
		return;
	}

	Log::Warning("{} に、UnityEvent でつないだメソッド {} がありません（引数の型が変わったかもしれません）", type->GetName(), name);
}


std::vector<MonoBehavior::ListenerMethod> ou::MonoBehavior::FindListenerMethods() const {
	std::vector<ListenerMethod> methods;
	if (!instance_) { return methods; }

	auto *engine = ScriptEngine::sEngine_;
	const auto *type = instance_->GetObjectType();
	for (asUINT i = 0; i < type->GetMethodCount(); i++) {
		const auto *method = type->GetMethodByIndex(i);
		PersistentListenerMode mode{};
		if (FindListenerMode(engine, method, mode)) { methods.push_back({method->GetName(), mode}); }
	}
	return methods;
}
