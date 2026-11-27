#pragma once

#include "Core/Object.h"
#include "Graphics/GraphicsTypes.h"
#include "Math/Matrix4x4.h"
#include "Math/Vector4.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>


namespace ou {

// 前方宣言
class GraphicsBuffer;
class Texture;

class ComputeShader final: public Object {
	friend class ComputeShaderImporter;

public: // メンバ関数

	int FindKernel(std::string_view name) const;
	bool HasKernel(std::string_view name) const;
	void GetKernelThreadGroupSizes(int kernelIndex, uint32_t &x, uint32_t &y, uint32_t &z) const;

	void SetFloat(std::string_view name, float value);
	void SetInt(std::string_view name, int value);
	void SetBool(std::string_view name, bool value);
	void SetVector(std::string_view name, const Vector4 &value);
	void SetMatrix(std::string_view name, const Matrix4x4 &value);

	void SetTexture(int kernelIndex, std::string_view name, Texture *texture);
	void SetBuffer(int kernelIndex, std::string_view name, GraphicsBuffer *buffer);

	void Dispatch(int kernelIndex, int threadGroupsX, int threadGroupsY, int threadGroupsZ);


private: // インナークラス

	struct GlobalVariable {
		std::string name;
		uint32_t offset = 0;
		uint32_t size = 0;
	};

	struct ResourceBinding {
		std::string name;
		uint32_t offset = 0;
		Texture *texture = nullptr;
		GraphicsBuffer *buffer = nullptr;
	};

	struct Kernel {
		std::string name;
		Graphics::Shader shader;
		std::array<uint32_t, 3> threadGroupSize{};
		std::vector<ResourceBinding> bindings;
	};


private: // メンバ変数

	std::vector<Kernel> kernels_;
	std::vector<GlobalVariable> globalVariables_;
	std::vector<std::byte> globals_;


private: // メンバ関数

	const GlobalVariable *FindGlobalVariable(std::string_view name) const;
	void WriteGlobal(std::string_view name, const void *data, uint32_t size);
	void SetResource(int kernelIndex, std::string_view name, Texture *texture, GraphicsBuffer *buffer);
	bool IsValidKernel(int kernelIndex) const;
};

} // namespace ou
