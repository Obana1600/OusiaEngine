#pragma once

#include "Core/Guid.h"
#include "Core/Object.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


// 前方宣言
namespace ou {
class AssetImporter;
} // namespace ou


// ファイルから読み込んだリソースをキャッシュする
namespace ou::AssetDatabase {

// アセットを指す情報
struct AssetReference {
	Guid guid;
	std::string subAsset; // メインのアセットなら空
};


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

// Assets と EngineAssets を走査して、GUID の表を作り直す (エディタでは追加で .meta の作成・更新)
void Refresh();

// アセットの GUID を引く (なければ無効な Guid)
Guid AssetPathToGuid(std::string_view assetPath);

// GUID からアセットのパスを引く (なければ空の文字列)
std::string GuidToAssetPath(const Guid &guid);

// アセットのパスを実際のファイルのパスにする
std::optional<std::filesystem::path> AssetPathToFullPath(std::string_view assetPath);

// オブジェクトがどのアセットかを引く (アセットでなければ nullopt)
std::optional<AssetReference> FindAssetReference(const Object *asset);

// GUID とサブアセットの名前から読み込む (subAsset が空ならメイン)
std::shared_ptr<Object> LoadAsset(const Guid &guid, std::string_view subAsset = {});

// アセットを探し、フィルターに当てはまる GUID の配列を返す
std::vector<Guid> FindAssets(std::string_view filter);

} // namespace ou::AssetDatabase
