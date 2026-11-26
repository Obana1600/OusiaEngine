#pragma once

#include "Core/EnumFlags.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>


namespace ou {

// 前方宣言
class Material;
class Serializer;


enum class ScriptableRenderPassInput : uint32_t {
	None = 0,
	Depth = 1 << 0,
	Color = 1 << 2,
};

class FullScreenPassRendererFeature final {
public: // インナークラス

	enum class InjectionPoint : uint32_t {
		BeforeRenderingPostProcessing,
		AfterRenderingPostProcessing,
	};


public: // メンバ変数

	std::shared_ptr<Material> passMaterial_;
	InjectionPoint injectionPoint_ = InjectionPoint::AfterRenderingPostProcessing;
	bool fetchColorBuffer_ = true;
	ScriptableRenderPassInput requirements_ = ScriptableRenderPassInput::None;


public: // メンバ関数

	void Serialize(Serializer &serializer);

	void SetName(std::string_view name);
	const std::string &GetName() const;

	void SetActive(bool active);
	bool GetIsActive() const;


private: // メンバ変数

	std::string name_ = "FullScreenPass";
	bool isActive_ = true;
};

} // namespace ou



template <>
struct EnableBitmaskOperators<ou::ScriptableRenderPassInput> {
	static constexpr bool enable = true;
};
