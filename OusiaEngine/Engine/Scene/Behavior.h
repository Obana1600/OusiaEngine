#pragma once

#include "Scene/Component.h"


namespace ou {

class Behavior: public Component {
	friend class GameObject;

public: // メンバ関数

	virtual void OnEnable() {};
	virtual void OnDisable() {};

	void SetEnabled(bool value);
	bool IsEnabled() const;

	bool IsActiveAndEnabled() const;

	void Serialize(Serializer &serializer) override;


private: // メンバ変数

	bool isEnabled_ = true;
	bool wasActiveAndEnabled_ = false;


private: // メンバ関数

	bool IsEnabledSelf() const override;

	void RefreshActiveAndEnabled();

	void DisableForDestroy();
};

} // namespace ou
