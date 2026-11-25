#pragma once

#include "Math/Color.h"
#include "Math/Vector2.h"
#include "PostProcess/VolumeParameterBase.h"

#include <type_traits>
#include <utility>


namespace ou {

template <typename T>
class VolumeParameter: public VolumeParameterBase {
public: // メンバ関数

	explicit VolumeParameter(T value = T{}, bool overrideState = false);

	virtual void SetValue(T value);
	const T &GetValue() const;

	void Override(T value);


protected: // メンバ関数

	void Serialize(Serializer &serializer) override;
	void Interp(const VolumeParameterBase &target, float t) override;


protected: // メンバ変数

	T value_;
};


using BoolParameter = VolumeParameter<bool>;
using FloatParameter = VolumeParameter<float>;
using Vector2Parameter = VolumeParameter<Vector2>;
using ColorParameter = VolumeParameter<Color>;


class MinFloatParameter final: public VolumeParameter<float> {
public: // メンバ関数

	MinFloatParameter(float value, float min, bool overrideState = false);

	void SetValue(float value) override;

	float GetMin() const;

private: // メンバ変数

	float min_;
};


class ClampedFloatParameter final: public VolumeParameter<float> {
public: // メンバ関数

	ClampedFloatParameter(float value, float min, float max, bool overrideState = false);

	void SetValue(float value) override;

	float GetMin() const;
	float GetMax() const;

private: // メンバ変数

	float min_;
	float max_;
};



template <typename T>
inline VolumeParameter<T>::VolumeParameter(T value, bool overrideState)
	: VolumeParameterBase(overrideState),
	  value_(std::move(value)) {}


template <typename T>
inline void VolumeParameter<T>::SetValue(T value) {
	value_ = std::move(value);
}


template <typename T>
inline const T &VolumeParameter<T>::GetValue() const {
	return value_;
}


template <typename T>
inline void VolumeParameter<T>::Override(T value) {
	SetOverrideState(true);
	SetValue(std::move(value));
}


template <typename T>
inline void VolumeParameter<T>::Serialize(Serializer &serializer) {
	serializer.Field("overrideState", overrideState_);
	if constexpr (std::is_enum_v<T>) {
		serializer.Enum("value", value_);
	} else {
		serializer.Field("value", value_);
	}

	if (serializer.IsReading()) { SetValue(value_); }
}


template <typename T>
inline void VolumeParameter<T>::Interp(const VolumeParameterBase &target, float t) {
	const auto &to = static_cast<const VolumeParameter<T> &>(target).value_;

	if constexpr (std::is_same_v<T, float>) {
		SetValue(value_ + (to - value_) * t);
	} else if constexpr (std::is_same_v<T, Vector2>) {
		SetValue(value_ + (to - value_) * t);
	} else if constexpr (std::is_same_v<T, Color>) {
		SetValue(Color::LerpUnclamped(value_, to, t));
	} else {
		if (t > 0.0f) { SetValue(to); }
	}
}

} // namespace ou
