#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace ou {

// 前方宣言
class Button;
class MonoBehavior;
class Object;
class Serializer;
class ScriptBindings;
class UnityEventDrawer;

// Unity の PersistentListenerMode（Inspector でつないだメソッドに渡す引数の型）
enum class PersistentListenerMode : uint8_t {
	Void,
	Int,
	Float,
	String,
	Bool,
};

// Unity の UnityEvent（引数なし）
// Inspector でつないだもの（シーンに保存される）と、実行中に AddListener したもの（保存されない）の両方を呼ぶ
class UnityEvent final {
	friend class Button;
	friend class ScriptBindings;
	friend class UnityEventDrawer;

public: // エイリアス

	using Listener = std::function<void()>;


public: // メンバ関数

	// 実行中につなぐ。外すときは返した番号を渡す
	uint32_t AddListener(Listener listener);
	void RemoveListener(uint32_t id);

	// Unity と同じく、外すのは実行中につないだものだけ（Inspector でつないだものは残る）
	void RemoveAllListeners();

	// Inspector でつないだもの → 実行中につないだもの、の順に呼ぶ
	void Invoke() const;

	int32_t GetPersistentEventCount() const;
	std::string GetPersistentMethodName(int32_t index) const;
	Object *GetPersistentTarget(int32_t index) const;


private: // インナークラス

	// Unity の PersistentCall。対象は MonoBehavior（スクリプト）のメソッドだけ
	struct PersistentCall {
		uint32_t targetInstanceId = 0; // 消えた対象を指さないよう、ポインタではなく番号で持つ
		std::string methodName;
		PersistentListenerMode mode = PersistentListenerMode::Void;
		int32_t intArgument = 0;
		float floatArgument = 0.0f;
		std::string stringArgument;
		bool boolArgument = false;
	};

	// 実行中につないだもの。key はスクリプトの関数を外すときに探す目印（C++ からつないだものは空）
	struct RuntimeCall {
		uint32_t id = 0;
		Listener listener;
		std::pair<const void *, const void *> key{};
	};


private: // メンバ変数

	std::vector<PersistentCall> persistentCalls_;
	std::vector<RuntimeCall> runtimeCalls_;
	uint32_t nextId_ = 1;


private: // メンバ関数

	// 持ち主の Component の Serialize から呼ぶ
	void Serialize(Serializer &serializer, std::string_view name);

	// スクリプトの関数（delegate）は、同じ関数・同じオブジェクトなら RemoveListener で外せる
	void AddListener(Listener listener, std::pair<const void *, const void *> key);
	void RemoveListener(std::pair<const void *, const void *> key);

	static void InvokePersistentCall(const PersistentCall &call);
};

} // namespace ou
