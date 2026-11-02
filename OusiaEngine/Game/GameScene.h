#pragma once

#include "Scene/Component.h"
#include "Scene/Scene.h"


namespace ou {

void BuildGameScene(Scene &scene);

class QueryProbe final: public Component {
public:
	void Start() override;
	void FixedUpdate() override;

private:
	int step_ = 0;
};

} // namespace ou
