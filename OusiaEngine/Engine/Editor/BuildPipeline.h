#pragma once

#include <filesystem>


namespace ou {

// Unity の BuildPipeline。Player（ビルド済みの exe）にアセットを付けて、フォルダに書き出す
// C++ のビルドは要らない（チームの人のエディタからも書き出せる）
class BuildPipeline final {
public: // 静的メンバ関数

	BuildPipeline() = delete;

	// location に <productName>.exe・DLL・EngineAssets・Assets・ProjectSettings を置く。失敗したら理由をログに出して false
	static bool BuildPlayer(const std::filesystem::path &location);
};

} // namespace ou
