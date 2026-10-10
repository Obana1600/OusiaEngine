#pragma once

#include <cstdint>
#include <dxgiformat.h>


namespace ou {

// フレームバッファ数
inline constexpr uint32_t kFrameCount = 2;

// SRV ヒープの 0 番
inline constexpr uint32_t kFallbackTextureIndex = 0;

// バックバッファのフォーマット
inline constexpr DXGI_FORMAT kBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

// バックバッファに書き込むときの RTV のフォーマット
// FLIP 系のスワップチェーンは SRGB で作れないので、ビュー側で SRGB にする
inline constexpr DXGI_FORMAT kBackBufferRtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

// 深度バッファのフォーマット
inline constexpr DXGI_FORMAT kDepthFormat = DXGI_FORMAT_D32_FLOAT;
inline constexpr float kDepthClearValue = 1.0f;

} // namespace ou