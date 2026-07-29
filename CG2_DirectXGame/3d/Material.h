#pragma once

#include "math/Matrix4x4.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include <d3d12.h>
#include <memory>
#include <string>
#include <wrl.h>


class Material {
public: // インナークラス

	enum class LightMode {
		None,
		Lambert,
		HalfLambert,
	};

	struct ConstBufferData {
		Matrix4x4 uvTransform;
		Vector3 color;
		float alpha;
		int lightMode; // 0: none, 1: lambert, 2; halfLambert
	};


public: // 静的メンバ関数

	static std::unique_ptr<Material> Create();


public: // メンバ変数

	std::string name_;

	Vector3 color_;
	float alpha_;

	Vector2 uvOffset_{};
	Vector2 uvScale_ = {1.0f, 1.0f};
	float uvRotate_{};

	int lightMode_ = static_cast<int>(LightMode::HalfLambert);

	std::string textureFileName_;


public: // メンバ関数

	ID3D12Resource *GetConstBuffer() const;

	void LoadTexture(const std::string &directoryPath);

	void Update();

	void SetGraphicsCommand(ID3D12GraphicsCommandList *commandList, UINT rootParameterIndexMaterial, UINT rootParameterIndexTexture);

	void SetGraphicsCommand(ID3D12GraphicsCommandList *commandList, UINT rootParameterIndexMaterial, UINT rootParameterIndexTexture, uint32_t textureHandle);

	uint32_t GetTextureHandle() const {
		return textureHandle_;
	}


private: // メンバ変数

	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;

	ConstBufferData *constMap_ = nullptr;

	uint32_t textureHandle_ = 0;

	static inline const std::string kFallbackTextureName_ = "white1x1.png";


private: // メンバ関数

	Material()
		: color_({1.0f, 1.0f, 1.0f}), alpha_(1.0f) {
	}

	void Initialize();

	void CreateConstBuffer();
};