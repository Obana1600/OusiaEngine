#pragma once

#include "Math/Vector2.h"

#include <cstdint>


namespace ou {

// 前方宣言
class GameObject;
class GraphicRaycaster;

enum class MoveDirection : uint8_t {
	Left,
	Up,
	Right,
	Down,
	None,
};

struct RaycastResult {
	GameObject *gameObject = nullptr;
	GraphicRaycaster *module = nullptr;

	int32_t depth = 0;
};

struct BaseEventData {
	virtual ~BaseEventData() = default;
};

struct PointerEventData: BaseEventData {
	enum class InputButton : uint8_t {
		Left,
		Right,
		Middle,
	};

	Vector2 position;
	InputButton button = InputButton::Left;

	RaycastResult pointerCurrentRaycast{};

	GameObject *pointerEnter = nullptr;
	GameObject *pointerPress = nullptr;
	GameObject *rawPointerPress = nullptr;

	bool eligibleForClick = false;
};

struct AxisEventData: BaseEventData {
	MoveDirection moveDir = MoveDirection::None;
};

} // namespace ou
