#pragma once

#include <cstdint>
#include <d3d12.h>
#include <string_view>
#include <wrl/client.h>


namespace ou {

class GpuResourceOld;
class GraphicsContext;


class CommandContext {
public: // 静的メンバ関数

	static CommandContext &Begin();

public:	 // メンバ関数
private: // メンバ変数
private: //
};

} // namespace ou