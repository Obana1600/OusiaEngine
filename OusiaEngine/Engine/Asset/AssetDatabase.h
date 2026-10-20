#pragma once

#include "Core/Object.h"

#include <memory>
#include <string_view>


// 前方宣言
namespace ou {
class AssetImporter;
} // namespace ou


// ファイルから読み込んだリソースをキャッシュする
namespace ou::AssetDatabase {

// アセットの読み込み (ex: "Assets/..." or "EngineAssets/...")
std::shared_ptr<Object> LoadMainAssetAtPath(std::string_view assetPath);

// 読み込んだアセットを T にキャストする
template <typename T>
std::shared_ptr<T> LoadAssetAtPath(std::string_view assetPath) {
	return std::dynamic_pointer_cast<T>(LoadMainAssetAtPath(assetPath));
}

// インポーターの取得 (アセットの設定を変更できる)
AssetImporter *GetImporterAtPath(std::string_view assetPath);

// アセットをインポートし直す
void ImportAsset(std::string_view assetPath);

// 使われていないアセットを解放する (インポーターは残す)
void UnloadUnusedAssets();

// キャッシュをすべて捨てる
void UnloadAllAssets();

} // namespace ou::AssetDatabase
