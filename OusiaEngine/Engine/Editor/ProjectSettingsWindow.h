#pragma once

#include "Editor/EditorWindow.h"
#include "Scene/LayerMask.h"

#include <array>
#include <cstdint>
#include <string>


namespace ou {

// Unity の Edit > Project Settings。左に項目の一覧、右にその中身
// 値を変えるとすぐ効き、欄から出たときに ProjectSettings/ のファイルへ書く（Play 中に変えても、Stop で戻らない）
class ProjectSettingsWindow final: public EditorWindow {
public: // インナークラス

	enum class Section : uint8_t {
		Physics,
		Player,
		TagsAndLayers,
		Time,
	};


public: // 静的メンバ関数

	// ウィンドウを開いて、section を出す（Build Settings の「Player Settings...」から）
	static void Open(Section section);


public: // メンバ関数

	ProjectSettingsWindow();


protected: // メンバ関数

	void OnGui() override;


private: // メンバ変数

	Section section_ = Section::Player;

	// 打っている途中の値（書き換え終わったら入れる）
	std::string productName_;
	int screenSize_[2] = {};
	std::array<std::string, LayerMask::kLayerCount> layerNames_;


private: // メンバ関数

	void DrawSectionList();

	void DrawPhysics();
	// Unity の Layer Collision Matrix（名前のあるレイヤーどうしが当たるか）
	void DrawLayerCollisionMatrix();
	void DrawPlayer();
	void DrawTagsAndLayers();
	void DrawTime();
};

} // namespace ou
