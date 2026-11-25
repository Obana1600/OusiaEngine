#pragma once

#include "Core/Object.h"
#include "PostProcess/VolumeParameter.h"

#include <memory>
#include <span>
#include <string_view>
#include <vector>


namespace ou {

class VolumeComponent: public Object {
	friend class VolumeProfile;
	friend class VolumeProfileEditor;
	friend class VolumeStack;

public: // メンバ関数

	VolumeComponent(const VolumeComponent &) = delete;
	VolumeComponent &operator=(const VolumeComponent &) = delete;
	~VolumeComponent() override = default;

	void Serialize(Serializer &serializer) override;

	void SetActive(bool active);
	bool GetActive() const;

	void SetAllOverridesTo(bool state);
	void Override(VolumeComponent &state, float interpFactor) const;

	std::span<VolumeParameterBase *const> GetParameters() const;


protected: // メンバ関数

	VolumeComponent() = default;

	void AddParameter(std::string_view name, VolumeParameterBase &parameter);


private: // メンバ変数

	bool active_ = true;
	std::vector<VolumeParameterBase *> parameters_;
	std::vector<std::string_view> parameterNames_;


private: // メンバ関数

	static std::unique_ptr<VolumeComponent> Create(std::string_view typeName);

	static std::string_view FindTypeName(const VolumeComponent &component);

	static std::vector<std::string_view> GetTypeNames();
};

} // namespace ou
