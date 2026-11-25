#pragma once

#include "Core/Object.h"
#include "PostProcess/VolumeComponent.h"

#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>


namespace ou {

class VolumeProfile final: public Object {
public: // メンバ関数

	void Serialize(Serializer &serializer) override;

	VolumeComponent *Add(std::string_view typeName, bool overrides = false);
	void Remove(std::string_view typeName);
	bool Has(std::string_view typeName) const;

	template <typename T>
	T *TryGet() const;

	const std::vector<std::unique_ptr<VolumeComponent>> &GetComponents() const;


private: // メンバ変数

	std::vector<std::unique_ptr<VolumeComponent>> components_;


private: // メンバ関数

	void ReadComponents(Serializer &serializer, size_t count);
};



template <typename T>
inline T *VolumeProfile::TryGet() const {
	static_assert(std::is_base_of_v<VolumeComponent, T>, "T は VolumeComponent の派生クラスでなければいけません");

	for (const auto &comp : components_) {
		if (auto *found = dynamic_cast<T *>(comp.get())) { return found; }
	}
	return nullptr;
}

} // namespace ou
