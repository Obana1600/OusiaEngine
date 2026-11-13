#pragma once

#include "Math/Color.h"
#include "Math/Quaternion.h"
#include "Math/Rect.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

#include <cstdint>
#include <memory>
#include <string>
#include <variant>


namespace ou {

// 前方宣言
class Object;

using FieldValue = std::variant<bool, int32_t, uint32_t, float, Vector2, Vector3, Vector4, Quaternion,
								Color, Rect, std::string, size_t, Object *, std::shared_ptr<Object>>;

struct FieldChange {
	std::string path;
	FieldValue value;
};

} // namespace ou
