#pragma once

#include "Core/Object.h"

// TODO: ライフサイクルを MonoBehavior に移す

namespace ou {

// 前方宣言
class GameObject;
class Transform;
class Collider;
class Collision;
class ControllerColliderHit;

class Component: public Object {
	friend class GameObject;

public: // メンバ関数

	virtual void Awake() {};
	virtual void Start() {};
	virtual void FixedUpdate() {};
	virtual void Update() {};
	virtual void LateUpdate() {};
	virtual void OnDestroy() {};

	// Unity の OnApplicationQuit。ゲームを閉じるとき（エディタでは Play を止めるとき）に、OnDisable・OnDestroy より先に呼ぶ
	virtual void OnApplicationQuit() {};

	virtual void OnTriggerEnter(Collider *) {};
	virtual void OnTriggerStay(Collider *) {};
	virtual void OnTriggerExit(Collider *) {};

	virtual void OnCollisionEnter(const Collision &) {};
	virtual void OnCollisionStay(const Collision &) {};
	virtual void OnCollisionExit(const Collision &) {};
	virtual void OnControllerColliderHit(const ControllerColliderHit &) {};

	// Unity の OnValidate。エディタだけで、Inspector で値を変えたときと、シーンを読んだときに呼ぶ
	// Awake より先に呼ばれることもあるので、ほかのコンポーネントがまだ準備できていなくても落ちないように書く
	virtual void OnValidate() {};

	// Unity の OnDrawGizmos / OnDrawGizmosSelected。エディタの Scene ビューを描くたびに呼ぶ（Play 中でも）
	// この中でだけ Gizmos で描ける。Selected は、この GameObject か親を選んでいるときだけ呼ぶ
	virtual void OnDrawGizmos() {};
	virtual void OnDrawGizmosSelected() {};

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
