#pragma once

#include "Graphics/GraphicsTypes.h"


namespace ou {

class ImmediateCommands final {
	friend class Engine;
	friend class GraphicsBuffer;
	friend class ComputeShader;

public: // メンバ関数

	ImmediateCommands() = delete;


private: // 静的メンバ関数

	static Graphics::CommandList Acquire();

	static void OnSubmitted();


private: // 静的メンバ変数

	static inline Graphics::CommandList sCmd_;
};

} // namespace ou