#pragma once

#include "DirectionLight.h"
#include "Material.h"
#include "Mesh.h"
#include <Windows.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>

class WorldTransform;
class Camera;


class ModelCommon {
public: // 静的メンバ関数

	static ModelCommon *GetInstance();

	static void Finalize();


public: // メンバ関数

	void Initialize();

	void TransformCommand(const WorldTransform &worldTransform, const Camera &camera);

	void PreDraw(ID3D12GraphicsCommandList *commandList);
	void PostDraw();

	DirectionLight &GetDirectionLight() {
		return directionLight_;
	}


private: // メンバ変数

	UINT descriptorHandleIncrementSize_ = 0;

	ID3D12GraphicsCommandList *commandList_ = nullptr;

	Microsoft::WRL::ComPtr<IDxcBlob> vsBlob_;
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob_;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

	DirectionLight directionLight_{};


private: // メンバ関数

	ModelCommon() = default;
	~ModelCommon() = default;
	ModelCommon(const ModelCommon &) = delete;
	ModelCommon operator=(const ModelCommon &) = delete;


	void CompileShaders();
	void CreateRootSignature();
	void CreatePipelineState();
};



class Model {
public: // インナークラス

	enum class RootParameter {
		WorldTransform,
		Camera,
		Material,
		Light,
		Texture,

		Count,
	};


public: // 静的メンバ関数

	static void StaticInitialize();
	static void StaticFinalize();

	static Model *Create();
	static Model *CreateFromObj(const std::string &modelName);

	static void PreDraw();
	static void PostDraw();


public: // メンバ関数

	void Draw(const WorldTransform &worldTransform, const Camera &camera);
	void Draw(const WorldTransform &worldTransform, const Camera &camera, uint32_t textureHandle);

	const std::unordered_map<std::string, std::unique_ptr<Material>> &GetMaterials() const {
		return materials_;
	}


private: // メンバ変数

	std::string name_;

	std::vector<std::unique_ptr<Mesh>> meshes_;

	std::unordered_map<std::string, std::unique_ptr<Material>> materials_;

	static inline const std::string kBaseDirectory = "resources/";


private: // メンバ関数

	Model() = default;

	void InitializeFromFile(const std::string &modelName);
	void LoadModel(const std::string &modelName);
	void LoadMaterial(const std::string &directoryPath, const std::string &fileName);
	void LoadTexture();
};