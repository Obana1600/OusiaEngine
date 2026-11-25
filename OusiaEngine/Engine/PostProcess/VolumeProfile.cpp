#include "VolumeProfile.h"
#include "Diagnostics/Log.h"

#include <algorithm>
#include <string>

using namespace ou;


void ou::VolumeProfile::Serialize(Serializer &serializer) {
	auto count = components_.size();
	if (!serializer.BeginArray("components", count)) { return; }

	if (serializer.IsReading()) {
		ReadComponents(serializer, count);
	} else {
		for (const auto &comp : components_) {
			serializer.Struct("", [&] {
				auto typeName = std::string(VolumeComponent::FindTypeName(*comp));
				serializer.Field("$type", typeName);
				comp->Serialize(serializer);
			});
		}
	}
	serializer.EndArray();
}


VolumeComponent *ou::VolumeProfile::Add(std::string_view typeName, bool overrides) {
	if (Has(typeName)) { return nullptr; }

	auto comp = VolumeComponent::Create(typeName);
	if (!comp) { return nullptr; }

	comp->SetAllOverridesTo(overrides);
	return components_.emplace_back(std::move(comp)).get();
}


void ou::VolumeProfile::Remove(std::string_view typeName) {
	std::erase_if(components_, [&](const auto &comp) {
		return VolumeComponent::FindTypeName(*comp) == typeName;
	});
}


bool ou::VolumeProfile::Has(std::string_view typeName) const {
	return std::any_of(components_.begin(), components_.end(), [&](const auto &comp) {
		return VolumeComponent::FindTypeName(*comp) == typeName;
	});
}


const std::vector<std::unique_ptr<VolumeComponent>> &ou::VolumeProfile::GetComponents() const {
	return components_;
}


void ou::VolumeProfile::ReadComponents(Serializer &serializer, size_t count) {
	std::vector<std::unique_ptr<VolumeComponent>> components;

	for (size_t i = 0; i < count; i++) {
		serializer.Struct("", [&] {
			std::string typeName;
			serializer.Field("$type", typeName);

			const auto isDuplicate = std::any_of(components.begin(), components.end(), [&](const auto &comp) {
				return VolumeComponent::FindTypeName(*comp) == typeName;
			});
			if (isDuplicate) {
				Log::Warning("VolumeProfile に同じ効果が 2 つあります: {}", typeName);
				return;
			}

			auto it = std::find_if(components_.begin(), components_.end(), [&](const auto &comp) {
				return comp && VolumeComponent::FindTypeName(*comp) == typeName;
			});
			auto component = (it != components_.end()) ? std::move(*it) : VolumeComponent::Create(typeName);
			if (!component) {
				Log::Warning("定義されていない型です: \"{}\"", typeName);
				return;
			}

			component->Serialize(serializer);
			components.push_back(std::move(component));
		});
	}

	components_ = std::move(components);
}
