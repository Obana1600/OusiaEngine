#pragma once

#include <cstdint>


namespace ou {

// 前方宣言
class GameObject;

class Selection final {
public: // 静的メンバ関数

	Selection() = delete;

	static void SetActiveGameObject(GameObject *gameObject);
	static GameObject *GetActiveGameObject();


private: // 静的メンバ変数

	static inline uint32_t sActiveInstanceId_ = 0;
};

} // namespace ou