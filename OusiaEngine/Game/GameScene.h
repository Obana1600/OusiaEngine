#pragma once

#include "Scene/Component.h"
#include "Scene/Scene.h"

#include <string>
#include <utility>
#include <vector>


namespace ou {

class RectTransform;

void BuildGameScene(Scene &scene);

// Rect を Unity 合わせにした確認用。数フレーム後に RectTransform の Rect をログに出し、少し待って終了する
class RectLayoutChecker final: public Component {
public:

	void Update() override;

	std::vector<std::pair<std::string, RectTransform *>> targets;
};

} // namespace ou
