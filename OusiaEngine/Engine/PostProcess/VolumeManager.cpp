#include "VolumeManager.h"
#include "Math/Mathf.h"
#include "PostProcess/Volume.h"
#include "PostProcess/VolumeProfile.h"
#include "Scene/GameObject.h"

#include <algorithm>

using namespace ou;


VolumeStack &ou::VolumeManager::GetStack() {
	static VolumeStack stack;
	return stack;
}


void ou::VolumeManager::Register(Volume *volume) {
	sVolumes_.push_back(volume);
}


void ou::VolumeManager::Unregister(Volume *volume) {
	std::erase(sVolumes_, volume);
}


void ou::VolumeManager::Update(LayerMask layerMask) {
	auto &stack = GetStack();
	stack.Reset();

	auto volumes = sVolumes_;
	std::stable_sort(volumes.begin(), volumes.end(), [](const Volume *a, const Volume *b) {
		return a->GetPriority() < b->GetPriority();
	});

	for (const auto *volume : volumes) {
		if ((layerMask.value & (1u << volume->GetGameObject()->GetLayer())) == 0) { continue; }

		const auto &profile = volume->GetSharedProfile();
		if (!profile) { continue; }

		if (!volume->GetIsGlobal()) { continue; }

		const auto weight = Mathf::Clamp01(volume->GetWeight());
		if (weight <= 0.0f) { continue; }

		for (const auto &component : profile->GetComponents()) {
			if (!component->GetActive()) { continue; }
			if (auto *state = stack.Find(*component)) { component->Override(*state, weight); }
		}
	}
}
