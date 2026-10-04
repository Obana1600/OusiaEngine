#pragma once

#include "Math/Matrix4x4.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include <d3d12.h>
#include <string>
#include <wrl.h>


namespace ou {

class Sprite {
public: // インナークラス

	struct VertexPosUv {
		Vector3 pos;
		Vector2 uv;
	};

	struct ConstBufferData {
		Matrix4x4 mat;
		Matrix4x4 uvTransform;
		Vector4 color;
	};


public: // 静的メンバ関数

	static void StaticInitialize(
		ID3D12Device *device,
		int windowWidth,
		int WidowHeight,
		const std::wstring &directoryPath = L"resources/");

	static void PreDraw(ID3D12GraphicsCommandList *commandList = nullptr);

	static void PostDraw();

	static Sprite *Create(
		uint32_t textureHandle,
		Vector2 position,
		Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f},
		Vector2 anchorPoint = {0.0f, 0.0f},
		bool isFlipX = false,
		bool isFlipY = false);


private: // 静的メンバ変数

	static const int kNumVertex = 4;

	static inline UINT sDescriptorHeapIncrementSize_{};

	static inline Microsoft::WRL::ComPtr<ID3D12RootSignature> sRootSignature_{};
	static inline Microsoft::WRL::ComPtr<ID3D12PipelineState> sPipelineState_{};

	static inline Matrix4x4 sMatProjection_{};

	static inline ID3D12Device *sDevice_ = nullptr;
	static inline ID3D12GraphicsCommandList *sCommandList_ = nullptr;


public: // メンバ関数

	bool Initialize();

	void SetTextureHandle(uint32_t textureHandle);

	uint32_t GetTextureHandle() const {
		return textureHandle_;
	}

	void SetTranslation(const Vector2 &translation);

	const Vector2 &GetTranslation() const {
		return translation_;
	}

	void SetRotation(const Vector3 &rotation);

	const Vector3 &GetRotation() const {
		return rotation_;
	}

	void SetScale(const Vector2 &scale);

	const Vector2 &GetScale() const {
		return scale_;
	}

	void SetUVTranslation(const Vector2 &translation) {
		uvTranslation_ = translation;
	}

	const Vector2 &GetUVTranslation() const {
		return uvTranslation_;
	}

	void SetUVRotation(float rotation) {
		uvRotation_ = rotation;
	}

	float GetUVRotation() const {
		return uvRotation_;
	}

	void SetUVScale(const Vector2 &scale) {
		uvScale_ = scale;
	}

	const Vector2 &GetUVScale() const {
		return uvScale_;
	}

	void SetSize(const Vector2 &size);

	const Vector2 &GetSize() const {
		return size_;
	}

	void SetAnchorPoint(const Vector2 &anchorPoint);

	const Vector2 &GetAnchorPoint() const {
		return anchorPoint_;
	}

	void SetColor(const Vector4 &color);

	const Vector4 &GetColor() const {
		return color_;
	}

	void SetIsFlipX(bool isFlipX);

	bool GetIsFlipX() const {
		return isFlipX_;
	}

	void SetIsFlipY(bool isFlipY);

	bool GetIsFlipY() const {
		return isFlipY_;
	}

	void SetTextureRect(const Vector2 &texBase, const Vector2 &texSize);

	void Draw();


private: // メンバ変数

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_{};
	Microsoft::WRL::ComPtr<ID3D12Resource> constResource_{};

	VertexPosUv *vertexData_ = nullptr;
	ConstBufferData *constData_ = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	UINT textureHandle_ = 0;

	Vector2 translation_{};
	Vector3 rotation_{};
	Vector2 scale_ = {1.0f, 1.0f};

	Vector2 uvTranslation_{};
	float uvRotation_{};
	Vector2 uvScale_ = {1.0f, 1.0f};

	Vector2 size_{};

	Vector2 anchorPoint_{};

	Matrix4x4 worldMatrix_{};

	Vector4 color_{};

	bool isFlipX_ = false;
	bool isFlipY_ = false;

	Vector2 texBase_{};
	Vector2 texSize_{};

	D3D12_RESOURCE_DESC resourceDesc_{};


private: // メンバ関数

	Sprite();
	Sprite(
		uint32_t textureHandle,
		const Vector2 &position,
		const Vector2 &size,
		const Vector4 &color,
		const Vector2 &anchorPoint,
		bool isFlipX,
		bool isFlipY);

	void TransferVertex();
};

} // namespace ou