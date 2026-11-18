#pragma once


namespace ou::Application {

bool IsPlaying();

// エディタの Pause で、このフレームのゲームの更新を止めているか（ゲームのビルドでは常に false）
bool IsPaused();

void Quit();

bool IsQuitRequested();

} // namespace ou::Application
