#include "TweenManager.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"

#include <algorithm>
#include <utility>

using namespace ou;


void ou::TweenManager::Update(float deltaTime, float unscaledDeltaTime) {
	sIsUpdating_ = true;

	const auto count = static_cast<uint32_t>(sSlots_.size());
	for (uint32_t i = 0; i < count; i++) {
		auto &entry = sSlots_[i];
		if (!entry.isAlive) { continue; }

		auto &tween = entry.tween;

		const auto *owner = static_cast<const Component *>(Object::FindObjectFromInstanceId(tween.ownerId));
		if (!owner || owner->IsDestroyed() || owner->GetGameObject()->IsDestroyed()) {
			Kill(i);
			continue;
		}

		if (!tween.isPlaying) { continue; }

		const auto hasFinished = tween.Advance(tween.usesUnscaledTime ? unscaledDeltaTime : deltaTime);

		if (hasFinished && entry.isAlive) { Finish(i); }
	}

	sIsUpdating_ = false;
	ReleaseKilledEntries();
}


void ou::TweenManager::Finalize() {
	sSlots_.clear();
	sFreeIndices_.clear();
	sKilledIndices_.clear();
}


void ou::TweenManager::KillAll(const Component *owner) {
	if (!owner) { return; }

	const auto ownerId = owner->GetInstanceId();
	for (uint32_t i = 0; i < sSlots_.size(); i++) {
		if (sSlots_[i].isAlive && sSlots_[i].tween.ownerId == ownerId) { Kill(i); }
	}
}


TweenHandle ou::TweenManager::Add(Component *owner, std::function<void()> captureStartValue, std::function<void(float)> applyValue, float duration) {
	if (!owner) {
		Log::Error("TweenManager::To の owner が null です");
		return {};
	}

	uint32_t index = 0;
	if (sIsUpdating_ || sFreeIndices_.empty()) {
		index = static_cast<uint32_t>(sSlots_.size());
		sSlots_.emplace_back();
	} else {
		index = sFreeIndices_.back();
		sFreeIndices_.pop_back();
	}

	auto &slot = sSlots_[index];
	slot.isAlive = true;
	slot.tween.ownerId = owner->GetInstanceId();
	slot.tween.captureStartValue = std::move(captureStartValue);
	slot.tween.applyValue = std::move(applyValue);
	slot.tween.duration = Mathf::Max(duration, 0.0f);

	return TweenHandle(index, slot.generation);
}


Tween *ou::TweenManager::FindTween(uint32_t index, uint32_t generation) {
	if (index >= sSlots_.size()) { return nullptr; }

	auto &slot = sSlots_[index];
	if (!slot.isAlive || slot.generation != generation) { return nullptr; }
	if (!Object::FindObjectFromInstanceId(slot.tween.ownerId)) { return nullptr; }

	return &slot.tween;
}


void ou::TweenManager::Finish(uint32_t index) {
	auto &tween = sSlots_[index].tween;
	tween.isPlaying = false;
	tween.isCompleted = true;

	const auto onComplete = tween.onComplete;
	if (tween.autoKill) { Kill(index); }
	if (onComplete) { onComplete(); }
}


void ou::TweenManager::Kill(uint32_t index) {
	auto &slot = sSlots_[index];
	slot.isAlive = false;
	slot.generation++;

	sKilledIndices_.push_back(index);
}


void ou::TweenManager::ReleaseKilledEntries() {
	for (const auto index : sKilledIndices_) {
		sSlots_[index].tween = Tween();
		sFreeIndices_.push_back(index);
	}
	sKilledIndices_.clear();
}
