#pragma once

#include "Serialize/Serializer.h"


namespace ou {

class VolumeParameterBase {
	friend class VolumeComponent;

public: // メンバ関数

	VolumeParameterBase() = default;
	VolumeParameterBase(const VolumeParameterBase &) = delete;
	VolumeParameterBase &operator=(const VolumeParameterBase &) = delete;
	virtual ~VolumeParameterBase() = default;

	void SetOverrideState(bool overrideState);
	bool GetOverrideState() const;


protected: // メンバ関数

	explicit VolumeParameterBase(bool overrideState);

	virtual void Serialize(Serializer &serializer) = 0;
	virtual void Interp(const VolumeParameterBase &target, float t) = 0;


protected: // メンバ変数

	bool overrideState_ = false;
};

} // namespace ou
