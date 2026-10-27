#include "CanvasUpdateRegistry.h"
#include "UI/Graphic.h"

#include <algorithm>
#include <utility>

using namespace ou;


void ou::CanvasUpdateRegistry::RegisterCanvasElementForGraphicRebuild(Graphic *graphic) {
	if (std::ranges::find(sGraphicRebuildQueue_, graphic) != sGraphicRebuildQueue_.end()) { return; }
	sGraphicRebuildQueue_.push_back(graphic);
}


void ou::CanvasUpdateRegistry::UnRegisterCanvasElementForRebuild(Graphic *graphic) {
	std::erase(sGraphicRebuildQueue_, graphic);
}


void ou::CanvasUpdateRegistry::PerformUpdate() {
	auto queue = std::move(sGraphicRebuildQueue_);
	sGraphicRebuildQueue_.clear();

	for (auto *graphic : queue) {
		graphic->Rebuild();
	}
}
