#include "UnityEvent.h"
#include "Script/MonoBehavior.h"
#include "Serialize/Serializer.h"

#include <algorithm>

using namespace ou;


uint32_t ou::UnityEvent::AddListener(Listener listener) {
	AddListener(std::move(listener), {});
	return nextId_ - 1;
}


void ou::UnityEvent::RemoveListener(uint32_t id) {
	std::erase_if(runtimeCalls_, [id](const RuntimeCall &call) { return call.id == id; });
}


void ou::UnityEvent::RemoveAllListeners() {
	runtimeCalls_.clear();
}


void ou::UnityEvent::Invoke() const {
	// 呼んだ先でつなぎ直されてもよいように、写してから呼ぶ
	const auto persistentCalls = persistentCalls_;
	for (const auto &call : persistentCalls) {
		InvokePersistentCall(call);
	}

	const auto runtimeCalls = runtimeCalls_;
	for (const auto &call : runtimeCalls) {
		call.listener();
	}
}


int32_t ou::UnityEvent::GetPersistentEventCount() const {
	return static_cast<int32_t>(persistentCalls_.size());
}


std::string ou::UnityEvent::GetPersistentMethodName(int32_t index) const {
	if (index < 0 || index >= GetPersistentEventCount()) { return {}; }
	return persistentCalls_[index].methodName;
}


Object *ou::UnityEvent::GetPersistentTarget(int32_t index) const {
	if (index < 0 || index >= GetPersistentEventCount()) { return nullptr; }
	return Object::FindObjectFromInstanceId(persistentCalls_[index].targetInstanceId);
}


void ou::UnityEvent::Serialize(Serializer &serializer, std::string_view name) {
	serializer.Array(name, persistentCalls_, [&](PersistentCall &call) {
		serializer.Struct("", [&] {
			// 対象は番号で持つので、読み書きのときだけポインタにする
			auto *target = dynamic_cast<MonoBehavior *>(Object::FindObjectFromInstanceId(call.targetInstanceId));
			serializer.Reference("target", target);
			if (serializer.IsReading()) { call.targetInstanceId = target ? target->GetInstanceId() : 0; }

			serializer.Field("methodName", call.methodName);
			serializer.Enum("mode", call.mode);
			serializer.Field("intArgument", call.intArgument);
			serializer.Field("floatArgument", call.floatArgument);
			serializer.Field("stringArgument", call.stringArgument);
			serializer.Field("boolArgument", call.boolArgument);
		});
	});
}


void ou::UnityEvent::AddListener(Listener listener, std::pair<const void *, const void *> key) {
	runtimeCalls_.push_back({nextId_++, std::move(listener), key});
}


void ou::UnityEvent::RemoveListener(std::pair<const void *, const void *> key) {
	// Unity と同じく、同じものを何度もつないでいたら、外すのは 1 つだけ
	const auto it = std::find_if(runtimeCalls_.begin(), runtimeCalls_.end(), [&key](const RuntimeCall &call) { return call.key == key; });
	if (it != runtimeCalls_.end()) { runtimeCalls_.erase(it); }
}


void ou::UnityEvent::InvokePersistentCall(const PersistentCall &call) {
	if (call.methodName.empty()) { return; }

	// 対象が消えていたら呼ばない（Unity も、消えた対象への呼び出しは飛ばす）
	auto *target = dynamic_cast<MonoBehavior *>(Object::FindObjectFromInstanceId(call.targetInstanceId));
	if (!target || target->IsDestroyed()) { return; }

	target->InvokeListenerMethod(call.methodName, call.mode, call.intArgument, call.floatArgument, call.stringArgument, call.boolArgument);
}
