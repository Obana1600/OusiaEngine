#pragma once

#include "Object.h"


namespace ou {

// 前方宣言
class GameObject;
class Transform;

class Component: public Object {
	friend class GameObject;

public: // メンバ関数

	virtual void Awake() {};

	virtual void Start() {};

	virtual void Update() {};

	virtual void LateUpdate() {};

	virtual void OnDestroy() {};

	GameObject *GetGameObject() const;
	Transform *GetTransform() const;


private: // メンバ変数

	bool isStarted_ = false;

	GameObject *gameObject_ = nullptr;
	Transform *transform_ = nullptr;
};

} // namespace ou
