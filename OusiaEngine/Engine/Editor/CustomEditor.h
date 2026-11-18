#pragma once

#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>


namespace ou {

// 前方宣言
class Editor;
class Object;

class CustomEditor final {
	friend class Editor;

public: // コンストラクタ

	CustomEditor() = delete;


private: // エイリアス

	using Factory = std::unique_ptr<Editor> (*)();


private: // 静的メンバ変数

	static inline std::unordered_map<std::type_index, Factory> sFactories_;


private: // 静的メンバ関数

	template <typename TEditor, typename TTarget>
	static void Register();

	static void RegisterEngineEditors();

	static std::unique_ptr<Editor> Create(const Object &target);
};



template <typename TEditor, typename TTarget>
inline void CustomEditor::Register() {
	static_assert(std::is_base_of_v<Editor, TEditor>, "TEditor は Editor の派生クラスでなければいけません");
	static_assert(std::is_base_of_v<Object, TTarget>, "TTarget は Object の派生クラスでなければいけません");

	sFactories_[typeid(TTarget)] = []() -> std::unique_ptr<Editor> { return std::make_unique<TEditor>(); };
}

} // namespace ou
