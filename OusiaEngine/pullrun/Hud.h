#pragma once

#include "Scene/Component.h"

#include <string>


namespace ou {

// 前方宣言
class Image;
class Text;

// 画面の表示（プレイヤーの HP・コンボ・ボスの HP・中央のメッセージ・操作説明）
class Hud final: public Component {
public: // 静的メンバ関数

	static Hud *GetInstance() { return sInstance_; }


public: // メンバ関数

	void Awake() override;
	void Start() override;
	void LateUpdate() override;
	void OnDestroy() override;

	// 中央に大きく出すメッセージ。duration 秒で消える（0 以下で出し続ける）
	void ShowBanner(const std::string &message, float duration);


private: // 静的メンバ変数

	static inline Hud *sInstance_ = nullptr;


private: // メンバ変数

	Image *playerBar_ = nullptr;
	Text *comboText_ = nullptr;
	Image *bossBar_ = nullptr;
	Text *bannerText_ = nullptr;
	Text *helpText_ = nullptr;
	float bannerTimer_ = 0.0f;
};

} // namespace ou
