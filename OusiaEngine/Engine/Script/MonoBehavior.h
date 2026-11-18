#pragma once

#include "Core/JsonFile.h"
#include "Scene/Behavior.h"
#include "Events/UnityEvent.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>


// 前方宣言
class asIScriptFunction;
class asIScriptObject;


namespace ou {

class MonoBehavior: public Behavior {
	friend class MonoScript;
	friend class InspectorWindow;
	friend class ScriptBindings;
	friend class UnityEvent;
	friend class UnityEventDrawer;

public: // メンバ関数

	MonoBehavior() = default;
	~MonoBehavior() override;

	void Awake() override;
	void Start() override;
	void FixedUpdate() override;
	void Update() override;
	void LateUpdate() override;
	void OnDestroy() override;

	void OnEnable() override;
	void OnDisable() override;

	void OnCollisionEnter(const Collision &collision) override;
	void OnCollisionStay(const Collision &collision) override;
	void OnCollisionExit(const Collision &collision) override;

	void OnTriggerEnter(Collider *other) override;
	void OnTriggerStay(Collider *other) override;
	void OnTriggerExit(Collider *other) override;

	void Serialize(Serializer &serializer) override;


private: // インナークラス

	// スクリプトのメソッド
	struct Methods {
		asIScriptFunction *awake = nullptr;
		asIScriptFunction *start = nullptr;
		asIScriptFunction *fixedUpdate = nullptr;
		asIScriptFunction *update = nullptr;
		asIScriptFunction *lateUpdate = nullptr;
		asIScriptFunction *onDestroy = nullptr;
		asIScriptFunction *onEnable = nullptr;
		asIScriptFunction *onDisable = nullptr;

		asIScriptFunction *onCollisionEnter = nullptr;
		asIScriptFunction *onCollisionStay = nullptr;
		asIScriptFunction *onCollisionExit = nullptr;
		asIScriptFunction *onTriggerEnter = nullptr;
		asIScriptFunction *onTriggerStay = nullptr;
		asIScriptFunction *onTriggerExit = nullptr;
	};


	// Inspector の UnityEvent でつなげるメソッド（void で、引数は 0 か 1 つ）
	struct ListenerMethod {
		std::string name;
		PersistentListenerMode mode = PersistentListenerMode::Void;
	};


private: // メンバ変数

	std::shared_ptr<MonoScript> script_;
	asIScriptObject *instance_ = nullptr;
	Methods methods_;
	bool isAwakeCalled_ = false;

	Json savedJson_;


private: // メンバ関数

	void SetScript(std::shared_ptr<MonoScript> script);

	void CreateInstance();
	void ReleaseInstance();

	void CallMessage(asIScriptFunction *method);
	void CallCollisionMessage(asIScriptFunction *method, const Collision &collision);
	void CallTriggerMessage(asIScriptFunction *method, Collider *other);

	// UnityEvent から呼ぶ。名前と引数の型が合うメソッドを探して呼ぶ（なければ警告）
	void InvokeListenerMethod(std::string_view name, PersistentListenerMode mode, int32_t intArgument, float floatArgument, const std::string &stringArgument, bool boolArgument);
	std::vector<ListenerMethod> FindListenerMethods() const;
};

} // namespace ou
