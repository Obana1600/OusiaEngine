#pragma once

#include "Core/Object.h"
#include "Scene/Component.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <vector>


namespace ou::TypeRegistry {

using Factory = std::function<std::unique_ptr<Component>()>;

void RegisterType(std::type_index type, std::string_view name, Factory factory);

template <typename T>
void Register(std::string_view name);

std::vector<std::string> GetTypeNames();
const std::string *FindName(const Object &object);

std::unique_ptr<Component> Create(std::string_view name);



template <typename T>
void Register(std::string_view name) {
	static_assert(std::is_base_of_v<Component, T>, "T は Component の派生クラスでなければいけません");
	static_assert(std::is_default_constructible_v<T>, "T は引数なしのコンストラクタでなければいけません");

	RegisterType(typeid(T), name,
				 []() -> std::unique_ptr<Component> { return std::make_unique<T>(); });
}

} // namespace ou::TypeRegistry
