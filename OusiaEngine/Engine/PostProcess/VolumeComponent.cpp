#include "VolumeComponent.h"
#include "PostProcess/Builtin/ColorAdjustments.h"
#include "PostProcess/Builtin/Tonemapping.h"

#include <typeindex>

using namespace ou;


namespace {

struct ComponentType {
	std::string_view name;
	std::type_index type;
	std::unique_ptr<VolumeComponent> (*create)();
};

template <typename T>
ComponentType MakeComponentType(std::string_view name) {
	return {name,
			typeid(T),
			[]() -> std::unique_ptr<VolumeComponent> { return std::make_unique<T>(); }};
}

const std::vector<ComponentType> &GetComponentTypes() {
	static const std::vector<ComponentType> kTypes = {
		MakeComponentType<Tonemapping>("Tonemapping"),
		MakeComponentType<ColorAdjustments>("ColorAdjustments"),
	};
	return kTypes;
}

} // namespace



void ou::VolumeComponent::Serialize(Serializer &serializer) {
	serializer.Field("active", active_);
	for (size_t i = 0; i < parameters_.size(); i++) {
		serializer.Struct(parameterNames_[i], [&] { parameters_[i]->Serialize(serializer); });
	}
}


void ou::VolumeComponent::SetActive(bool active) {
	active_ = active;
}


bool ou::VolumeComponent::GetActive() const {
	return active_;
}


void ou::VolumeComponent::SetAllOverridesTo(bool state) {
	for (auto *parameter : parameters_) {
		parameter->SetOverrideState(state);
	}
}


void ou::VolumeComponent::Override(VolumeComponent &state, float interpFactor) const {
	if (typeid(state) != typeid(*this)) { return; }

	for (size_t i = 0; i < parameters_.size(); i++) {
		const auto *from = parameters_[i];
		if (!from->GetOverrideState()) { continue; }

		auto *to = state.parameters_[i];
		to->SetOverrideState(true);
		to->Interp(*from, interpFactor);
	}
}


std::span<VolumeParameterBase *const> ou::VolumeComponent::GetParameters() const {
	return parameters_;
}


void ou::VolumeComponent::AddParameter(std::string_view name, VolumeParameterBase &parameter) {
	parameters_.push_back(&parameter);
	parameterNames_.push_back(name);
}


std::unique_ptr<VolumeComponent> ou::VolumeComponent::Create(std::string_view typeName) {
	for (const auto &type : GetComponentTypes()) {
		if (type.name != typeName) { continue; }

		auto component = type.create();
		component->SetName(typeName);
		return component;
	}

	return nullptr;
}


std::string_view ou::VolumeComponent::FindTypeName(const VolumeComponent &component) {
	for (const auto &type : GetComponentTypes()) {
		if (type.type == typeid(component)) { return type.name; }
	}
	return {};
}


std::vector<std::string_view> ou::VolumeComponent::GetTypeNames() {
	std::vector<std::string_view> names;
	for (const auto &type : GetComponentTypes()) {
		names.push_back(type.name);
	}
	return names;
}
