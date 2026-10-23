#pragma once

#include "Scene/Component.h"
#include "Scene/Scene.h"

#include <memory>
#include <string>


namespace ou {

// 前方宣言
class FontAsset;
class Image;
class Text;

void BuildGameScene(Scene &scene);

// テキストの確認: ImGui の「Text」ウィンドウで、1 つの Text の設定を全部触れる
class TextPlayground final: public Component {
public: // メンバ関数

	void Start() override;
	void Update() override;

	void SetTargets(Text *text, Image *frame, Text *counter);


private: // メンバ変数

	Text *text_ = nullptr;
	Image *frame_ = nullptr;	// Text の RectTransform の範囲を見せる
	Text *counter_ = nullptr; // 毎フレーム文字列が変わる

	std::string editingText_;
	int fontIndex_ = 3; // Regular
	float rotation_ = 0.0f;
	float scale_ = 1.0f;
	bool isCounterRunning_ = true;


private: // メンバ関数

	void ApplyTransform();
	void ShowTextWindow();
};

} // namespace ou
