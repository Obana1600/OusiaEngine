#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>


namespace ou {

struct Guid {
public: // メンバ変数

	uint64_t high = 0;
	uint64_t low = 0;


public: // 静的メンバ変数

	static const Guid Invalid;


public: // 静的メンバ関数

	// GUID を生成する
	static Guid Generate();

	// 数の文字列を GUID に変換する
	static std::optional<Guid> Parse(std::string_view text);


public: // メンバ関数

	bool IsValid() const;

	std::string ToString() const;


public: // オペレータ

	friend bool operator==(const Guid &a, const Guid &b) = default;
};

} // namespace ou



template <>
struct std::hash<ou::Guid> {
	size_t operator()(const ou::Guid &guid) const noexcept {
		return std::hash<uint64_t>()(guid.high) ^ (std::hash<uint64_t>()(guid.low) << 1);
	}
};