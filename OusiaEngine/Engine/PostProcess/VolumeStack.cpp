#include "VolumeStack.h"

#include <typeinfo>

using namespace ou;


ou::VolumeStack::VolumeStack() {
	Reset();
}


void ou::VolumeStack::Reset() {
	components_.clear();
	for (const auto &typeName : VolumeComponent::GetTypeNames()) {
		components_.push_back(VolumeComponent::Create(typeName));
	}
}


VolumeComponent *ou::VolumeStack::Find(const VolumeComponent &component) const {
	for (const auto &candidate : components_) {
		if (typeid(*candidate) == typeid(component)) { return candidate.get(); }
	}
	return nullptr;
}
