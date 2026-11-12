#pragma once

#include "Animation/Tween/Tween.h"
#include "Animation/Tween/TweenHandle.h"
#include "Math/Mathf.h"
#include "Math/Quaternion.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>


namespace ou {

// 前方宣言
class Component;

class TweenManager final {
	friend class TweenHandle;

public: // 静的メンバ関数

	template <typename T>
	static TweenHandle To(Component *owner,
						  std::function<T()> getter, std::function<void(const T &)> setter,
						  const T &to, float duration);

	static void Update(float deltaTime, float unscaledDeltaTime);
	static void Finalize();

	static void KillAll(const Component *component);


private: // インナークラス

	struct Slot {
		Tween tween;
		uint32_t generation = 1;
		bool isAlive = false;
	};


private: // 静的メンバ変数

	static inline std::deque<Slot> sSlots_;
	static inline std::vector<uint32_t> sFreeIndices_;
	static inline std::vector<uint32_t> sKilledIndices_;
	static inline bool sIsUpdating_ = false;


private: // 静的メンバ関数

	template <typename T>
	static T Lerp(const T &a, const T &b, float t);

	static TweenHandle Add(Component *owner,
						   std::function<void()> captureStartValue, std::function<void(float)> applyValue,
						   float duration);
	static Tween *FindTween(uint32_t index, uint32_t generation);

	static void Finish(uint32_t index);
	static void Kill(uint32_t index);
	static void ReleaseKilledEntries();
};



template <typename T>
inline TweenHandle TweenManager::To(Component *owner, std::function<T()> getter, std::function<void(const T &)> setter, const T &to, float duration) {
	auto from = std::make_shared<T>();

	auto captureStartValue = [from, getter = std::move(getter)]() {
		*from = getter();
	};
	auto applyValue = [from, to, setter = std::move(setter)](float ratio) {
		setter(Lerp(*from, to, ratio));
	};

	return Add(owner, std::move(captureStartValue), std::move(applyValue), duration);
}


template <typename T>
inline T TweenManager::Lerp(const T &a, const T &b, float t) {
	if constexpr (std::is_same_v<T, float>) {
		return Mathf::LerpUnclamped(a, b, t);
	} else if constexpr (std::is_same_v<T, Quaternion>) {
		return Quaternion::SlerpUnclamped(a, b, t);
	} else {
		return T::LerpUnclamped(a, b, t);
	}
}

} // namespace ou
