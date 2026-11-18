#pragma once

#include "Core/Object.h"


namespace ou {

// 前方宣言
class MonoBehavior;

class MonoScript: public Object {
public: // 静的メンバ関数

	static MonoScript *FromMonoBehavior(MonoBehavior *behavior);
};

} // namespace ou
