#include "Guid.h"

#include <charconv>
#include <format>
#include <random>

using namespace ou;


namespace {

// 文字列の長さ
constexpr size_t kTextLength = 32;
constexpr size_t kHalfLength = 16;


bool ParseHalf(std::string_view text, uint64_t &out) {
	const auto *first = text.data();
	const auto *last = text.data() + text.size();
	const auto result = std::from_chars(first, last, out, 16);
	return result.ec == std::errc() && result.ptr == last;
}

} // namespace



const Guid ou::Guid::Invalid = {0, 0};


Guid ou::Guid::Generate() {
	std::random_device device;
	const auto next = [&device] {
		return (static_cast<uint64_t>(device()) << 32) | device();
	};

	Guid guid;
	do {
		guid.high = next();
		guid.low = next();
	} while (!guid.IsValid());

	return guid;
}


std::optional<Guid> ou::Guid::Parse(std::string_view text) {
	if (text.size() != kTextLength) { return std::nullopt; }

	Guid guid;
	if (!ParseHalf(text.substr(0, kHalfLength), guid.high) ||
		!ParseHalf(text.substr(kHalfLength), guid.low)) {
		return std::nullopt;
	}

	return guid;
}


bool ou::Guid::IsValid() const {
	return high != Invalid.high || low != Invalid.low;
}


std::string ou::Guid::ToString() const {
	return std::format("{:016x}{:016x}", high, low);
}
