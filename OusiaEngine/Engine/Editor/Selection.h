#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>


namespace ou {

// 前方宣言
class GameObject;
class Object;

// Unity の Selection。選んでいるのはシーンの GameObject か、Project のアセット（フォルダも含む）のどちらか 1 つ
class Selection final {
public: // 静的メンバ関数

	Selection() = delete;

	// Unity の Selection.activeObject。GameObject か、選んでいるアセット（読み込めないフォルダなどは nullptr）
	static Object *GetActiveObject();

	// シーンの GameObject を選ぶ（アセットの選択は外れる）
	static void SetActiveGameObject(GameObject *gameObject);
	static GameObject *GetActiveGameObject();

	// Project のアセットを選ぶ（GameObject の選択は外れる）。subAsset が空ならメインのアセット
	static void SetActiveAsset(std::string_view assetPath, std::string_view subAsset = {});
	static bool IsAssetSelected(std::string_view assetPath, std::string_view subAsset = {});

	// 選んでいるアセットのパス（アセットを選んでいなければ空）
	static const std::string &GetActiveAssetPath();
	static const std::string &GetActiveSubAsset();

	// 何も選んでいない状態にする
	static void Clear();


private: // 静的メンバ変数

	static inline uint32_t sActiveInstanceId_ = 0;

	static inline std::string sAssetPath_;
	static inline std::string sSubAsset_;
	// 選んでいる間は解放されないように持っておく（インポートし直すと別のオブジェクトになるので、毎回引き直す）
	static inline std::shared_ptr<Object> sAsset_;
	static inline bool sHasTriedLoad_ = false;


private: // 静的メンバ関数

	static std::shared_ptr<Object> Load();
};

} // namespace ou
