#include "GameScene.h"
#include "Core/Application.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Input/Input.h"
#include "Platform/Screen.h"
#include "Serialize/TypeRegistry.h"

using namespace ou;


// シーンは .scene から読むので、ここではゲームの型を登録するだけ
void ou::RegisterGameTypes() {
	TypeRegistry::Register<InputProbe>("InputProbe");
	TypeRegistry::Register<QuitAfterFrames>("QuitAfterFrames");
}


void ou::InputProbe::Update() {
	if (Input::GetKeyDown(KeyCode::Space)) { Log::Info("TEST Space down"); }
	if (Input::GetKeyDown(KeyCode::A)) { Log::Info("TEST A down"); }
	if (Input::GetMouseButtonDown(0)) {
		const auto position = Input::GetMousePosition();
		Log::Info("TEST click ({}, {}) screen {}x{}", position.x, position.y, Screen::GetWidth(), Screen::GetHeight());
	}
}


void ou::QuitAfterFrames::Update() {
	if (Time::GetTime() >= 1.0f) { Application::Quit(); }
}
