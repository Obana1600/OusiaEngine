#pragma once

#include "Scene/SceneState.h"

#include <memory>
#include <span>
#include <string_view>


namespace ou {

class AbstractSceneFactory {
public: // メンバ関数

	virtual ~AbstractSceneFactory() = default;

	virtual std::unique_ptr<SceneState> CreateScene(std::string_view name) const = 0;

	virtual std::span<const std::string_view> ListSceneNames() const = 0;
};

} // namespace ou
