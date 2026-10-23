#include "SceneFactory.h"
#include "GameScene.h"
#include "TitleScene.h"

using namespace ou;


namespace {

constexpr std::string_view kSceneNames[] = {
	"Title",
	"Game",
};

} // namespace



std::unique_ptr<SceneState> ou::SceneFactory::CreateScene(std::string_view name) const {
	if (name == "Title") { return std::make_unique<TitleScene>(); }
	if (name == "Game") { return std::make_unique<GameScene>(); }
	return nullptr;
}


std::span<const std::string_view> ou::SceneFactory::ListSceneNames() const {
	return kSceneNames;
}
