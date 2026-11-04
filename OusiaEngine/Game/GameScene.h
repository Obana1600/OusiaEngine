#pragma once

#include "Scene/Component.h"
#include "Scene/Scene.h"


namespace ou {

void BuildGameScene(Scene &scene);

// 当たったこと・離れたことをログに出す
class ContactLogger final: public Component {
public:
	void OnCollisionEnter(const Collision &collision) override;
	void OnCollisionExit(const Collision &collision) override;
	void OnTriggerEnter(Collider *other) override;
	void OnTriggerExit(Collider *other) override;
};

// 単位 2（衝突のフィルタ）の確かめ
class FilterProbe final: public Component {
public:
	GameObject *playerBox = nullptr;
	GameObject *enemyBox = nullptr;
	GameObject *ghost = nullptr;
	GameObject *visitor = nullptr;
	GameObject *sleeper = nullptr;
	GameObject *switcher = nullptr;

	void Start() override;
	void Update() override;

private:
	int step_ = 0;

	void LogPositions(const char *label) const;
	void LogQueries() const;
};

} // namespace ou
