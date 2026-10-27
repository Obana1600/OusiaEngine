#pragma once

#include "Scene/Component.h"

#include <string_view>


namespace ou {

// 前方宣言
class Button;
class EventSystem;
class Image;
class Text;

// クリアとゲームオーバーの画面。画面を暗くして、中央上に見出し、中央下に「リトライ」「タイトルへ」を出す
class ResultMenu final: public Component {
public: // 静的メンバ関数

	static ResultMenu *GetInstance() { return sInstance_; }


public: // メンバ関数

	void Awake() override;
	void Start() override;
	void Update() override;
	void OnDestroy() override;

	// delay 秒（実時間）待ってから、暗くしながら出す
	void Show(std::string_view title, std::string_view subtitle, float delay);


private: // 静的メンバ変数

	static inline ResultMenu *sInstance_ = nullptr;


private: // メンバ変数

	EventSystem *eventSystem_ = nullptr;
	Image *cover_ = nullptr;
	Text *titleText_ = nullptr;
	Text *subtitleText_ = nullptr;
	Button *retryButton_ = nullptr;
	Button *titleButton_ = nullptr;
	bool isShown_ = false;
	float delay_ = 0.0f;
	// 出し始めてからの時間
	float time_ = 0.0f;


private: // メンバ関数

	void SetMenuVisible(bool visible);
};

} // namespace ou
