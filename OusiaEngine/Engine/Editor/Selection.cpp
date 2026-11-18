#include "Selection.h"
#include "Asset/AssetDatabase.h"
#include "Scene/GameObject.h"

using namespace ou;


Object *ou::Selection::GetActiveObject() {
	if (auto *gameObject = GetActiveGameObject()) { return gameObject; }
	if (sAssetPath_.empty()) { return nullptr; }

	// フォルダ・対応していないファイルは選んだときに 1 回だけ調べる（毎フレームだとエラーのログが流れ続ける）
	if (!sHasTriedLoad_) {
		sHasTriedLoad_ = true;
		if (AssetDatabase::CanImport(sAssetPath_)) { sAsset_ = Load(); }
		return sAsset_.get();
	}

	// インポートし直すと別のオブジェクトになるので、読めているあいだは引き直す（キャッシュから引くだけなので軽い）
	if (sAsset_) { sAsset_ = Load(); }
	return sAsset_.get();
}


std::shared_ptr<Object> ou::Selection::Load() {
	if (sSubAsset_.empty()) { return AssetDatabase::LoadMainAssetAtPath(sAssetPath_); }
	return AssetDatabase::LoadAsset(AssetDatabase::AssetPathToGuid(sAssetPath_), sSubAsset_);
}


void ou::Selection::SetActiveGameObject(GameObject *gameObject) {
	sActiveInstanceId_ = gameObject ? gameObject->GetInstanceId() : 0;
	sAssetPath_.clear();
	sSubAsset_.clear();
	sAsset_.reset();
	sHasTriedLoad_ = false;
}


GameObject *ou::Selection::GetActiveGameObject() {
	if (sActiveInstanceId_ == 0) { return nullptr; }
	auto *gameObject = dynamic_cast<GameObject *>(Object::FindObjectFromInstanceId(sActiveInstanceId_));
	return (gameObject && !gameObject->IsDestroyed()) ? gameObject : nullptr;
}


void ou::Selection::SetActiveAsset(std::string_view assetPath, std::string_view subAsset) {
	sActiveInstanceId_ = 0;
	if (sAssetPath_ == assetPath && sSubAsset_ == subAsset) { return; }

	sAssetPath_ = assetPath;
	sSubAsset_ = subAsset;
	sAsset_.reset();
	sHasTriedLoad_ = false;
}


bool ou::Selection::IsAssetSelected(std::string_view assetPath, std::string_view subAsset) {
	return !sAssetPath_.empty() && sAssetPath_ == assetPath && sSubAsset_ == subAsset;
}


const std::string &ou::Selection::GetActiveAssetPath() {
	return sAssetPath_;
}


const std::string &ou::Selection::GetActiveSubAsset() {
	return sSubAsset_;
}


void ou::Selection::Clear() {
	SetActiveGameObject(nullptr);
}
