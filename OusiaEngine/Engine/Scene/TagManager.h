#pragma once

#include <string>
#include <string_view>
#include <vector>


namespace ou {

// Unity の Tags（ProjectSettings/TagManager.json の "tags"。レイヤーの名前は LayerMask が持つ）
// GameObject には、ここにあるタグだけを付けられる。組み込みのタグは消せない。Unity と同じく、名前は変えられない（足すか消すだけ）
class TagManager final {
	friend class Engine;

public: // 定数

	static constexpr std::string_view kUntagged = "Untagged";
	// ビルドのときに、このタグの GameObject と子を外す
	static constexpr std::string_view kEditorOnly = "EditorOnly";
	// Camera.main（SceneManager::GetMainCamera）が先に返すカメラ
	static constexpr std::string_view kMainCamera = "MainCamera";
	static constexpr std::string_view kPlayer = "Player";


public: // 静的メンバ関数

	TagManager() = delete;

	// 組み込みのタグ、足したタグの順
	static const std::vector<std::string> &GetTags();
	static bool IsBuiltinTag(std::string_view tag);
	static bool IsDefined(std::string_view tag);

	// 足して保存する。足せないなら、その理由を返す（足せたら空）
	static std::string AddTag(std::string_view tag);
	// 消して保存する（組み込みのタグは消せない）。このタグを付けている GameObject は、文字のまま残る
	static void RemoveTag(std::string_view tag);


private: // 静的メンバ関数

	static void Load();
	static void Save();
};

} // namespace ou
