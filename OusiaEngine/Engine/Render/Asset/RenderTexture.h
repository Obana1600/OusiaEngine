#pragma once

#include "Render/Asset/Texture.h"

#include <cstdint>


namespace ou {

// TODO: 増やす
enum class RenderTextureFormat : uint32_t {
	ARGB32 = 0,
	ARGBHalf = 2,
	Default = 7,
	DefaultHDR = 9,
	ARGBFloat = 11,
	RGFloat = 12,
	RFloat = 14,
	R8 = 16,
	BGRA32 = 20,
	RGB111110Float = 22,
};

class RenderTexture final: public Texture {
public: // メンバ関数

	RenderTexture(int width, int height, int depth, RenderTextureFormat format = RenderTextureFormat::Default);

	bool Create();
	void Release();
	bool IsCreated() const;

	void SetEnableRandomWrite(bool enableRandomWrite);
	bool GetEnableRandomWrite() const;

	RenderTextureFormat GetFormat() const;

	uint32_t GetWidth() const override;
	uint32_t GetHeight() const override;


private: // メンバ変数

	int width_ = 0;
	int height_ = 0;
	RenderTextureFormat format_ = RenderTextureFormat::Default;
	bool enableRandomWrite_ = false;
};

} // namespace ou
