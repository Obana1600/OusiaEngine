#include "VolumeManager.h"
#include "Math/Mathf.h"
#include "Physics/Collider.h"
#include "PostProcess/Volume.h"
#include "PostProcess/VolumeProfile.h"
#include "Render/GraphicsSettings.h"
#include "Scene/GameObject.h"

#include <algorithm>
#include <limits>

using namespace ou;


namespace {

float CalculateLocalFactor(const Volume &volume, const Vector3 &triggerPosition) {
	auto closestDistanceSqr = std::numeric_limits<float>::infinity();
	for (const auto *collider : volume.GetGameObject()->GetComponents<Collider>()) {
		if (!collider->IsEnabled()) { continue; }

		const auto closestPoint = collider->ClosestPoint(triggerPosition);
		closestDistanceSqr = std::min(closestDistanceSqr, (closestPoint - triggerPosition).LengthSquared());
	}

	const auto blendDistanceSqr = volume.GetBlendDistance() * volume.GetBlendDistance();
	if (closestDistanceSqr > blendDistanceSqr) { return 0.0f; }

	return blendDistanceSqr > 0.0f ? 1.0f - closestDistanceSqr / blendDistanceSqr : 1.0f;
}

} // namespace



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


void ou::VolumeManager::Update(const Vector3 &triggerPosition, LayerMask layerMask) {
	auto &stack = GetStack();
	stack.Reset();

	if (const auto &defaultProfile = GraphicsSettings::GetInstance().defaultVolumeProfile_) {
		OverrideData(stack, *defaultProfile, 1.0f);
	}

	auto volumes = sVolumes_;
	std::stable_sort(volumes.begin(), volumes.end(), [](const Volume *a, const Volume *b) {
		return a->GetPriority() < b->GetPriority();
	});

	for (const auto *volume : volumes) {
		if ((layerMask.value & (1u << volume->GetGameObject()->GetLayer())) == 0) { continue; }

		const auto &profile = volume->GetSharedProfile();
		if (!profile) { continue; }

		auto weight = Mathf::Clamp01(volume->GetWeight());
		if (weight <= 0.0f) { continue; }

		if (!volume->GetIsGlobal()) {
			const auto factor = CalculateLocalFactor(*volume, triggerPosition);
			if (factor <= 0.0f) { continue; }
			weight *= factor;
		}

		OverrideData(stack, *profile, weight);
	}
}


void ou::VolumeManager::OverrideData(VolumeStack &stack, const VolumeProfile &profile, float weight) {
	for (const auto &component : profile.GetComponents()) {
		if (!component->GetActive()) { continue; }
		if (auto *state = stack.Find(*component)) { component->Override(*state, weight); }
	}
}
