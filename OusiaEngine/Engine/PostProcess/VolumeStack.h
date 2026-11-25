#pragma once

#include "PostProcess/VolumeComponent.h"

#include <memory>
#include <type_traits>
#include <vector>


namespace ou {

class VolumeStack final {
	friend class VolumeManager;

public: // メンバ関数

	VolumeStack();

	template <typename T>
	T *GetComponent() const;


private: // メンバ変数

	std::vector<std::unique_ptr<VolumeComponent>> components_;


private: // メンバ関数

	void Reset();

	VolumeComponent *Find(const VolumeComponent &component) const;
};



template <typename T>
inline T *VolumeStack::GetComponent() const {
	static_assert(std::is_base_of_v<VolumeComponent, T>, "T は VolumeComponent の派生クラスでなければいけません");

	for (const auto &comp : components_) {
		if (auto *found = dynamic_cast<T *>(comp.get())) { return found; }
	}
	return nullptr;
}

} // namespace ou
