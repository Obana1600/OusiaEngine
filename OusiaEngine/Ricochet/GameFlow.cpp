#include "GameFlow.h"
#include "Boss.h"
#include "ComboCounter.h"
#include "Hud.h"
#include "Input/Input.h"
#include "Player.h"
#include "RicochetScene.h"
#include "Scene/SceneManager.h"
#include "Tuning.h"

#include <format>

using namespace ou;



void ou::GameFlow::Start() {
	if (auto *hud = Hud::GetInstance()) { hud->ShowBanner("ボスを倒せ", 2.0f); }
}


void ou::GameFlow::Update() {
	if (Input::GetKeyDown(KeyCode::R)) {
		SceneManager::LoadScene(kRicochetSceneName);
		return;
	}
	if (state_ != State::Playing) { return; }

	auto *player = Player::GetInstance();
	auto *boss = Boss::GetInstance();

#ifdef WITH_EDITOR
	// 確認用：F1 でボスの今の形態の HP を削り切る
	if (Input::GetKeyDown(KeyCode::F1) && boss) { boss->TakeDamage(boss->GetHp(), Vector2::Zero); }
#endif
	auto *hud = Hud::GetInstance();
	if (!player || !boss || !hud) { return; }

	if (boss->GetPhase() != lastPhase_) {
		lastPhase_ = boss->GetPhase();
		hud->ShowBanner("第二形態\n弱点（黄色）を狙え", Tuning::kBossTransitionTime);
	}

	if (boss->GetState() == BossState::Defeated) {
		state_ = State::Cleared;
		player->Freeze();
		const auto maxCombo = ComboCounter::GetInstance() ? ComboCounter::GetInstance()->GetMaxCombo() : 0;
		hud->ShowBanner(std::format("CLEAR!\n最大 {} コンボ　R でもう一度", maxCombo), 0.0f);
		return;
	}

	if (player->IsDead()) {
		state_ = State::GameOver;
		player->Freeze();
		hud->ShowBanner("GAME OVER\nR でリトライ", 0.0f);
	}
}
