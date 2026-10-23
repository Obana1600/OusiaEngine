#pragma once

#include <cstdint>


namespace ou {

// ゲームパッドのボタン
enum class GamepadButton : uint16_t {
	DpadUp = 0x0001,
	DpadDown = 0x0002,
	DpadLeft = 0x0004,
	DpadRight = 0x0008,
	Start = 0x0010,
	Select = 0x0020,
	LeftStick = 0x0040,
	RightStick = 0x0080,
	LeftShoulder = 0x0100,
	RightShoulder = 0x0200,
	South = 0x1000, // xbox: A
	East = 0x2000,	// xbox: B
	West = 0x4000,	// xbox: X
	North = 0x8000, // xbox: Y
};

} // namespace ou
