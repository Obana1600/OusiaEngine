#pragma once

#include "Scene/AbstractSceneFactory.h"


namespace ou {

class SceneFactory final: public AbstractSceneFactory {
public: // メンバ関数

	std::unique_ptr<SceneState> CreateScene(std::string_view name) const override;
	std::span<const std::string_view> ListSceneNames() const override;
};

} // namespace ou
