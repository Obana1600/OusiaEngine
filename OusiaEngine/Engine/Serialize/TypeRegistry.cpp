#include "TypeRegistry.h"
#include "Diagnostics/Log.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

using namespace ou;


namespace {

std::unordered_map<std::type_index, std::string> &NamesByType() {
	static std::unordered_map<std::type_index, std::string> names;
	return names;
}

std::unordered_map<std::string, TypeRegistry::Factory> &FactoriesByName() {
	static std::unordered_map<std::string, TypeRegistry::Factory> factories;
	return factories;
}

} // namespace



void ou::TypeRegistry::RegisterType(std::type_index type, std::string_view name, Factory factory) {
	const auto [it, inserted] = FactoriesByName().emplace(std::string(name), std::move(factory));
	if (!inserted) {
		Log::Error("既に同じ名前の型が登録されています: {}", name);
		return;
	}
	NamesByType().emplace(type, std::string(name));
}


std::vector<std::string> ou::TypeRegistry::GetTypeNames() {
	std::vector<std::string> names;
	for (const auto &[name, factory] : FactoriesByName()) {
		names.push_back(name);
	}
	std::sort(names.begin(), names.end());
	return names;
}


const std::string *ou::TypeRegistry::FindName(const Object &object) {
	const auto it = NamesByType().find(typeid(object));
	return it != NamesByType().end() ? &it->second : nullptr;
}


std::unique_ptr<Component> ou::TypeRegistry::Create(std::string_view name) {
	const auto it = FactoriesByName().find(std::string(name));
	return it != FactoriesByName().end() ? it->second() : nullptr;
}
