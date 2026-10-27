#include "GameFlow.h"
#include "Boss.h"
#include "ComboCounter.h"
#include "GameAudio.h"
#include "Hud.h"
#include "Input/Input.h"
#include "Player.h"
#include "ResultMenu.h"
#include "Tuning.h"

#include <format>

using namespace ou;


namespace {

// 勝敗が決まってから、リザルトの画面を出すまでの時間（撃破や被弾の演出を見せる）
constexpr float kResultDelay = 1.2f;

} // namespace



void ou::GameFlow::Start() {
	GameAudio::PlayBgm(Bgm::BossNormal);
	if (auto *hud = Hud::GetInstance()) { hud->ShowBanner("マルっこ", 2.0f); }
}


void ou::GameFlow::Update() {
	if (state_ != State::Playing) { return; }

	auto *player = Player::GetInstance();
	auto *boss = Boss::GetInstance();

#ifdef WITH_EDITOR
	// 確認用：F1 でボスの今の形態の HP を削り切る
	if (Input::GetKeyDown(KeyCode::F1) && boss) { boss->TakeDamage(boss->GetHp(), Vector2::Zero); }
#endif
	if (!player || !boss) { return; }

	if (boss->GetPhase() != lastPhase_) {
		lastPhase_ = boss->GetPhase();
		GameAudio::PlaySe(Se::BossPowerUp);
		GameAudio::PlayBgm(Bgm::BossHard);
	}

	if (boss->GetState() == BossState::Defeated) {
		state_ = State::Cleared;
		player->Freeze();
		const auto maxCombo = ComboCounter::GetInstance() ? ComboCounter::GetInstance()->GetMaxCombo() : 0;
		GameAudio::PlayBgm(Bgm::Clear, false);
		if (auto *menu = ResultMenu::GetInstance()) { menu->Show("クリア", std::format("最大 {} コンボ", maxCombo), kResultDelay); }
		return;
	}

	if (player->IsDead()) {
		state_ = State::GameOver;
		player->Freeze();
		GameAudio::PlayBgm(Bgm::GameOver, false);
		if (auto *menu = ResultMenu::GetInstance()) { menu->Show("ゲームオーバー", "", kResultDelay); }
	}
}
