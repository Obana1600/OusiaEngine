#pragma once

#include "Core/Object.h"

// TODO: ライフサイクルを MonoBehavior に移す

namespace ou {

// 前方宣言
class GameObject;
class Transform;
class Collider;

class Component: public Object {
	friend class GameObject;

public: // メンバ関数

	virtual void Awake() {};
	virtual void Start() {};
	virtual void Update() {};
	virtual void LateUpdate() {};
	virtual void OnDestroy() {};

	virtual void OnTriggerEnter(Collider *) {};
	virtual void OnTriggerStay(Collider *) {};
	virtual void OnTriggerExit(Collider *) {};

	GameObject *GetGameObject() const;
	Transform *GetTransform() const;


private: // メンバ変数

	bool isStarted_ = false;
	bool isAwoken_ = false;

	GameObject *gameObject_ = nullptr;
	Transform *transform_ = nullptr;


private: // メンバ関数

	virtual bool IsEnabledSelf() const;
};

} // namespace ou
