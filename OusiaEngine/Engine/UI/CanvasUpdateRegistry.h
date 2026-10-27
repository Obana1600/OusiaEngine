#pragma once

#include <vector>


namespace ou {

// 前方宣言
class Graphic;

class CanvasUpdateRegistry final {
public: // 静的メンバ関数

	static void RegisterCanvasElementForGraphicRebuild(Graphic *graphic);
	static void UnRegisterCanvasElementForRebuild(Graphic *graphic);

	static void PerformUpdate();


private: // 静的メンバ変数

	static inline std::vector<Graphic *> sGraphicRebuildQueue_;
};

} // namespace ou
