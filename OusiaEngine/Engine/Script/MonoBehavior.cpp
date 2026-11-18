#include "MonoBehavior.h"
#include "Core/Application.h"
// #include "Physics/Collider.h"
// #include "Physics/Collision.h"
#include "Script/MonoScript.h"
#include "Script/ScriptEngine.h"
#include "Serialize/Serializer.h"

#include <utility>

#include <angelscript.h>

using namespace ou;


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
	methods_.onEnable = type->GetMethodByDecl("void OnEnable()");
	methods_.onDisable = type->GetMethodByDecl("void OnDisable()");
	methods_.onCollisionEnter = type->GetMethodByDecl("void OnCollisionEnter(Collision)");
	methods_.onCollisionStay = type->GetMethodByDecl("void OnCollisionStay(Collision)");
	methods_.onCollisionExit = type->GetMethodByDecl("void OnCollisionExit(Collision)");
	methods_.onTriggerEnter = type->GetMethodByDecl("void OnTriggerEnter(Collider@)");
	methods_.onTriggerStay = type->GetMethodByDecl("void OnTriggerStay(Collider@)");
	methods_.onTriggerExit = type->GetMethodByDecl("void OnTriggerExit(Collider@)");
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
