#pragma once

#include <string_view>


namespace ou {

// 前方宣言
class GameObject;
class Object;
class Scene;

// Unity の PrefabUtility（コピー方式なので、置いたものはプレハブとつながりを持たない）
class PrefabUtility final {
public: // 静的メンバ関数

	PrefabUtility() = delete;

	// プレハブのアセット（.prefab の一番上の親の GameObject）か
	static bool IsPartOfPrefabAsset(const Object *object);

	// Unity と同じく、名前に (Clone) を付けずにシーンへ置く
	static GameObject *InstantiatePrefab(GameObject *assetRoot, Scene *scene);

	// instanceRoot とその子を .prefab に書き、読み込んだプレハブのアセットを返す（同じパスがあれば上書き。失敗したら nullptr）
	static GameObject *SaveAsPrefabAsset(GameObject *instanceRoot, std::string_view assetPath);
};

} // namespace ou
