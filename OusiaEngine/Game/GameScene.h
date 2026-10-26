#pragma once

#include "Math/Vector2.h"
#include "Physics/BoxCollider.h"
#include "Physics/Physics.h"
#include "Physics/Rigidbody.h"
#include "Scene/Behaviour.h"
#include "Scene/Component.h"
#include "Scene/Scene.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


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
	Vector2 underlayOffset_{}; // ImGui で編集する値（Text 側は範囲に丸めて持つ）


private: // メンバ関数

	void ApplyTransform();
	void ShowTextWindow();
};

// 入力の確認: Down / Up・マウスを TEST のログに出し、ImGui の「Input」ウィンドウに今の状態を出す
class InputPlayground final: public Component {
public: // メンバ関数

	void Update() override;


private: // メンバ変数

	std::vector<std::string> events_; // 最近の Down / Up（新しいものが後ろ）
	Vector2 deltaTotal_{};
	Vector2 scrollTotal_{};

	bool hasCheckedRange_ = false; // 範囲外の player の確認を 1 回だけする
	bool wasConnected_[4] = {};
	float lowMotor_ = 0.0f;  // ImGui の振動のスライダー（player 0）
	float highMotor_ = 0.0f;


private: // メンバ関数

	void AddEvent(std::string text);
	void UpdateGamepads(uint64_t frame);
};

// FixedUpdate の確認: 呼ばれる順番と、1 秒あたりの回数を TEST のログに出す
class FixedUpdateProbe final: public Component {
public: // メンバ関数

	void Start() override;
	void FixedUpdate() override;
	void Update() override;
	void LateUpdate() override;


private: // メンバ変数

	float realTime_ = 0.0f;     // timeScale に関係ない経過時間
	float secondTimer_ = 0.0f;
	int fixedCount_ = 0;        // この 1 秒の FixedUpdate の回数
	int frameFixedCount_ = 0;   // このフレームの FixedUpdate の回数
	int maxFixedPerFrame_ = 0;
	int phase_ = -1;


private: // メンバ関数

	void ChangePhase(int phase);
};

// 調べものの確認: Raycast / RaycastAll / OverlapSphere / OverlapBox の結果を TEST のログに出す
class QueryProbe final: public Component {
public: // メンバ関数

	void Start() override;
	void FixedUpdate() override;


private: // メンバ変数

	int step_ = 0;
	BoxCollider *disabled_ = nullptr; // 2 歩目に無効にする


private: // メンバ関数

	BoxCollider *CreateBox(const char *name, const Vector3 &position, const Vector3 &size, Transform *parent = nullptr);
	void LogRaycastAll(const char *label, const Vector3 &origin, const Vector3 &direction, float maxDistance,
					   QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);
	void LogColliders(const char *label, const std::vector<Collider *> &colliders);
};

} // namespace ou
