#pragma once

#include "Core/EnumFlags.h"
#include "Diagnostics/Log.h"
#include "Graphics/GraphicsTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>


namespace ou {

class GraphicsBuffer final {
	friend class ComputeShader;

public: // インナークラス

	enum class Target : uint32_t {
		Vertex = 1 << 0,
		Index = 1 << 1,
		CopySource = 1 << 2,
		CopyDestination = 1 << 3,
		Structured = 1 << 4,
		Raw = 1 << 5,
	};


public: // メンバ関数

	GraphicsBuffer(Target target, int count, int stride);
	GraphicsBuffer(const GraphicsBuffer &) = delete;
	GraphicsBuffer &operator=(const GraphicsBuffer &) = delete;

	template <typename T>
	void SetData(const std::vector<T> &data);

	template <typename T>
	void SetData(const std::vector<T> &data, int managedBufferStartIndex, int graphicsBufferStartIndex, int count);

	void Release();
	bool IsValid() const;

	int GetCount() const;
	int GetStride() const;
	Target GetTarget() const;

	void SetName(std::string_view name);
	const std::string &GetName() const;


private: // メンバ変数

	Graphics::GpuBuffer buffer_;
	Target target_;
	int count_ = 0;
	int stride_ = 0;
	std::string name_;


private: // メンバ関数

	void SetDataBytes(const void *data, size_t byteOffset, size_t byteSize);
};



template <typename T>
inline void GraphicsBuffer::SetData(const std::vector<T> &data) {
	SetData(data, 0, 0, static_cast<int>(data.size()));
}


template <typename T>
inline void GraphicsBuffer::SetData(const std::vector<T> &data, int managedBufferStartIndex, int graphicsBufferStartIndex, int count) {
	static_assert(std::is_trivially_copyable_v<T>, "T はコピーできる型でなければいけません");

	if (managedBufferStartIndex < 0 || graphicsBufferStartIndex < 0 || count < 0 ||
		static_cast<size_t>(managedBufferStartIndex) + static_cast<size_t>(count) > data.size()) {
		Log::Error("GraphicsBuffer \"{}\" の SetData の範囲が data の外です", name_);
		return;
	}

	SetDataBytes(data.data() + managedBufferStartIndex,
				 static_cast<size_t>(graphicsBufferStartIndex) * sizeof(T),
				 static_cast<size_t>(count) * sizeof(T));
}

} // namespace ou



template <>
struct EnableBitmaskOperators<ou::GraphicsBuffer::Target> {
	static constexpr bool enable = true;
};
