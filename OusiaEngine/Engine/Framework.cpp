#include "Framework.h"
#include "Scene/SceneManager.h"

#include <utility>

using namespace ou;


ou::Framework::Framework(EngineDesc desc)
	: desc_(std::move(desc)) {}


void ou::Framework::Run() {
	Initialize();

	while (true) {
		Update();
		if (IsEndRequested()) { break; }

		Draw();
	}

	Finalize();
}


void ou::Framework::Initialize() {
	engine_.Initialize(desc_);
}


void ou::Framework::Finalize() {
	engine_.Finalize();
}


void ou::Framework::Update() {
	isEndRequested_ = engine_.Update();
	if (isEndRequested_) { return; }

	SceneManager::Update();
}


void ou::Framework::Draw() {
	auto cmd = engine_.BeginDraw();
	if (!cmd.IsValid()) { return; }

	SceneManager::Render(cmd);

	engine_.BeginOverlay(cmd);
	SceneManager::RenderOverlay(cmd);

	engine_.EndDraw(cmd);
}


bool ou::Framework::IsEndRequested() const {
	return isEndRequested_;
}
