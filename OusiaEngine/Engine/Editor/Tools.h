#pragma once


namespace ou {

enum class Tool {
	View,
	Move,
	Rotate,
	Scale,
	None,
};

enum class PivotRotation {
	Local,
	Global,
};


class Tools final {
public: // 静的メンバ関数

	Tools() = delete;

	static void SetCurrent(Tool tool);
	static Tool GetCurrent();

	static void SetPivotRotation(PivotRotation pivotRotation);
	static PivotRotation GetPivotRotation();


private: // 静的メンバ変数

	static inline Tool sCurrent_ = Tool::Move;
	static inline PivotRotation sPivotRotation_ = PivotRotation::Local;
};

} // namespace ou
