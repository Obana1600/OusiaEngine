#pragma once

#include "Core/EnumFlags.h"

#include <cstdint>


namespace ou::Graphics {

// デバッグレイヤーの強さ
enum class ValidationMode : uint8_t {
	Disabled, // 検証しない
	Enabled,  // デバッグレイヤー
	Gpu,	  // デバッグレイヤー + GPU ベース検証
	Verbose,  // デバッグレイヤー + 情報メッセージの出力
};


// コマンドキューの種類
// この値を直接配列のインデックスとして渡すため、enum class にはしない
enum QueueType {
	QUEUE_GRAPHICS, // ダイレクトキュー
	QUEUE_COMPUTE,	// コンピュートキュー
	QUEUE_COPY,		// コピーキュー

	QUEUE_COUNT,
};


enum class ShaderStage : uint8_t {
	MS,	 // メッシュシェーダ
	AS,	 // アンプリフィケーションシェーダ
	VS,	 // 頂点シェーダ
	HS,	 // ハルシェーダ
	DS,	 // ドメインシェーダ
	GS,	 // ジオメトリシェーダ
	PS,	 // ピクセルシェーダ
	CS,	 // コンピュートシェーダ
	LIB, // シェーダライブラリ

	Count,
};


// シェーダーモデル: 最小値は 6.6
enum class ShaderModel : uint8_t {
	SM_6_6,
	SM_6_7,
	SM_6_8,
	SM_6_9,
};


enum class PrimitiveTopology : uint8_t {
	UNDEFINED,	   // 未設定
	TRIANGLELIST,  // 3 頂点で 1 つの三角形
	TRIANGLESTRIP, // 連続した三角形。直前の 2 頂点を共有する
	POINTLIST,	   // 1 頂点で 1 つの点
	LINELIST,	   // 2 頂点で 1 本の線
	LINESTRIP,	   // 連続した線。直前の頂点を共有する
	PATCHLIST,	   // テッセレーション用
};


enum class ComparisonFunc : uint8_t {
	NEVER,		   // 常に失敗する
	LESS,		   // 小さければ通す
	EQUAL,		   // 等しければ通す
	LESS_EQUAL,	   // 小さいか等しければ通す
	GREATER,	   // 大きければ通す
	NOT_EQUAL,	   // 等しくなければ通す
	GREATER_EQUAL, // 大きいか等しければ大須
	ALWAYS,		   // 常に通す
};


enum class DepthWriteMask : uint8_t {
	ZERO, // 書き込まない
	ALL,  // 書き込む
};


// TODO: 追加できるらしい
enum class Blend : uint8_t {
	ZERO,			  // 0 を掛ける
	ONE,			  // 1 を掛ける
	SRC_COLOR,		  // 描く色を掛ける
	INV_SRC_COLOR,	  // (1 - 描く色) を掛ける
	SRC_ALPHA,		  // 描く色のアルファを掛ける
	INV_SRC_ALPHA,	  // (1 - 描く色のアルファ) を掛ける
	DEST_COLOR,		  // 既存の色を掛ける
	INV_DEST_COLOR,	  // (1 - 既存の色) を掛ける
	DEST_ALPHA,		  // 既存のアルファを掛ける
	INV_DEST_ALPHA,	  // (1 - 既存のアルファ) を掛ける
	SRC_ALPHA_SAT,	  // min(描く色のアルファ, 1 - 既存のアルファ) を掛ける
	BLEND_FACTOR,	  // 定数を掛ける
	INV_BLEND_FACTOR, // (1 - 定数) を掛ける
};


enum class BlendOp : uint8_t {
	ADD,		  // 描く色 + 既存の色
	SUBTRACT,	  // 描く色 - 既存の色
	REV_SUBTRACT, // 既存の色 - 描く色
	MIN,		  // min(描く色, 既存の色)
	MAX,		  // max(描く色, 既存の色)
};


enum class FillMode : uint8_t {
	WIREFRAME, // 三角形を線で描く
	SOLID,	   // 三角形を塗りつぶす
};


enum class CullMode : uint8_t {
	NONE,  // 両面とも描く
	FRONT, // 裏面だけ描く
	BACK,  // 表面だけ描く
};


enum class InputClassification : uint8_t {
	PER_VERTEX_DATA,   // 頂点ごとに次のデータに進む
	PER_INSTANCE_DATA, // インスタンスごとに次のデータへ進む
};


enum class HeapType : uint8_t {
	DEFAULT,  // GPU 専用
	UPLOAD,	  // CPU -> GPU
	READBACK, // GPU -> CPU
};


// UV 座標が 0 ~ 1 の範囲外になったときにテクスチャをどう扱うか
enum class TextureAddressMode : uint8_t {
	WRAP,		 // 繰り返す
	MIRROR,		 // 反転する
	CLAMP,		 // 端のピクセルを引き延ばす
	BORDER,		 // 範囲外を指定した境界色で塗る
	MIRROR_ONCE, // -1 ~ 1 の範囲で 1 回だけ反転し、その範囲外を CLAMP する
};


// TODO: まだ追加できる
enum class Filter : uint8_t {
	MIN_MAG_MIP_POINT,			   // テクスチャを補完しない
	MIN_MAG_MIP_LINEAR,			   // テクスチャを補完する
	ANISOTROPIC,				   // 異方性フィルタリング
	COMPARISON_MIN_MAG_MIP_LINEAR, //
};


// 1 頂点を何ビットで表すか
enum class IndexBufferFormat : uint8_t {
	UINT16, // 1 頂点につき 2 バイト
	UINT32, // 1 頂点につき 4 バイト
};


// リソースを GPU からどう見るか
enum class ViewType : uint8_t {
	SRV, // シェーダから読み取り専用
	UAV, // シェーダから読み書き
	RTV, // レンダーターゲット
	DSV, // 深度ステンシルバッファ
};


// TODO: ポストプロセスに必要なフォーマットあり
enum class Format : uint8_t {
	UNKNOWN, //

	R32G32B32A32_FLOAT, // float4
	R32G32B32_FLOAT,	// float3
	R32G32_FLOAT,		// float2
	R32_FLOAT,			// float
	R32_UINT,			//
	R16_UINT,			//
	R8_UNORM,			//

	R8G8B8A8_UNORM,		 // 8 ビットカラー
	R8G8B8A8_UNORM_SRGB, // 8 ビットカラー + ガンマ補正
	B8G8R8A8_UNORM,		 // 8 ビット BGRA
	B8G8R8A8_UNORM_SRGB, // 8 ビット BGRA + ガンマ補正

	D32_FLOAT,		   // 32 ビット深度バッファ
	D24_UNORM_S8_UINT, // 24 ビット深度バッファ + 8 ビットステンシル
};

enum class ColorWrite : uint8_t {
	DISABLE = 0,
	ENABLE_RED = 1 << 0,
	ENABLE_GREEN = 1 << 1,
	ENABLE_BLUE = 1 << 2,
	ENABLE_ALPHA = 1 << 3,
	ENABLE_ALL = ENABLE_RED | ENABLE_GREEN | ENABLE_BLUE | ENABLE_ALPHA,
};


//
enum class BindFlag : uint32_t {
	NONE = 0,
	VERTEX_BUFFER = 1 << 0,	   // 頂点バッファ
	INDEX_BUFFER = 1 << 1,	   // インデックスバッファ
	CONSTANT_BUFFER = 1 << 2,  // 定数バッファ
	SHADER_RESOURCE = 1 << 3,  // SRV
	RENDER_TARGET = 1 << 4,	   // RTV
	DEPTH_STENCIL = 1 << 5,	   // DSV
	UNORDERED_ACCESS = 1 << 6, // UAV
};


enum class ResourceMiscFlag : uint32_t {
	NONE = 0,
	TEXTURECUBE = 1 << 0,
	BUFFER_RAW = 1 << 1,
	BUFFER_STRUCTURED = 1 << 2,
};


enum class ResourceState : uint32_t {
	UNDEFINED = 0,
	SHADER_RESOURCE = 1 << 0,
	SHADER_RESOURCE_COMPUTE = 1 << 1,
	UNORDERED_ACCESS = 1 << 2,
	COPY_SRC = 1 << 3,
	COPY_DST = 1 << 4,
	RENDERTARGET = 1 << 5,
	DEPTHSTENCIL = 1 << 6,
	DEPTHSTENCIL_READONLY = 1 << 7,
	VERTEX_BUFFER = 1 << 8,
	INDEX_BUFFER = 1 << 9,
	CONSTANT_BUFFER = 1 << 10,
	INDIRECT_ARGUMENT = 1 << 11,
};


// TextureAddressMode::BORDER の色
enum class SamplerBorderColor : uint8_t {
	TRANSPARENT_BLACK, // {0, 0, 0, 0}
	OPAQUE_BLACK,	   // {0, 0, 0, 1}
	OPAQUE_WHITE,	   // {1, 1, 1, 1}
};

// TODO: Stencil 関係の enum

} // namespace ou::Graphics



// ビット演算を有効化する列挙子
template <>
struct EnableBitmaskOperators<ou::Graphics::ColorWrite> {
	static constexpr bool enable = true;
};

template <>
struct EnableBitmaskOperators<ou::Graphics::BindFlag> {
	static constexpr bool enable = true;
};

template <>
struct EnableBitmaskOperators<ou::Graphics::ResourceMiscFlag> {
	static constexpr bool enable = true;
};

template <>
struct EnableBitmaskOperators<ou::Graphics::ResourceState> {
	static constexpr bool enable = true;
};
