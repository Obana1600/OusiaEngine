#include "PrefabUtility.h"
#include "Asset/AssetDatabase.h"
#include "Core/JsonFile.h"
#include "Diagnostics/Log.h"
#include "Scene/GameObject.h"
#include "Serialize/SceneSerializer.h"

#include <string>

using namespace ou;


bool ou::PrefabUtility::IsPartOfPrefabAsset(const Object *object) {
	return dynamic_cast<const GameObject *>(object) && AssetDatabase::FindAssetReference(object).has_value();
}


GameObject *ou::PrefabUtility::InstantiatePrefab(GameObject *assetRoot, Scene *scene) {
	if (!IsPartOfPrefabAsset(assetRoot) || !scene) { return nullptr; }

	// プレハブの一番上の親は parent が null で書かれているので、置き場所はそのまま（呼んだ側が親を付ける）
	auto *instance = SceneSerializer::Instantiate(*assetRoot, *scene, [](GameObject &) {});
	if (instance) { instance->SetName(assetRoot->GetName()); }
	return instance;
}


GameObject *ou::PrefabUtility::SaveAsPrefabAsset(GameObject *instanceRoot, std::string_view assetPath) {
	if (!instanceRoot || !instanceRoot->GetScene()) { return nullptr; }

	const auto fullPath = AssetDatabase::AssetPathToFullPath(assetPath);
	if (!fullPath) {
		Log::Error("プレハブを書けないパスです: {}", assetPath);
		return nullptr;
	}

	// シーンの外を指す欄は null（アセットは guid）にして、シーンから独立させる
	if (!JsonFile::Save(*fullPath, SceneSerializer::ToJson(*instanceRoot, false))) {
		Log::Error("プレハブを書けませんでした: {}", assetPath);
		return nullptr;
	}

	// 新しいファイルなら .meta を作る。上書きなら、読み込み済みのものを作り直す
	// 古いプレハブを持っている欄は、そのまま古いものを指す（同じ guid で保存されるので、開き直すと新しいものになる）
	AssetDatabase::Refresh();
	AssetDatabase::ImportAsset(assetPath);

	auto asset = AssetDatabase::LoadAssetAtPath<GameObject>(assetPath);
	if (!asset) { return nullptr; }

	Log::Info("プレハブを保存しました: {}", assetPath);
	return asset.get(); // AssetDatabase が持ち続ける
}
