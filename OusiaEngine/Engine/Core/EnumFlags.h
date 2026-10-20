#pragma once

#include <type_traits>


// 列挙子のビット演算を有効化する構造体
template <typename E>
struct EnableBitmaskOperators {
	static constexpr bool enable = false;
};


template <typename E>
constexpr std::enable_if_t<EnableBitmaskOperators<E>::enable, E> operator|(E a, E b) {
	using U = std::underlying_type_t<E>;
	return static_cast<E>(static_cast<U>(a) | static_cast<U>(b));
}


template <typename E>
constexpr std::enable_if_t<EnableBitmaskOperators<E>::enable, E &> operator|=(E &a, E b) {
	a = a | b;
	return a;
}


template <typename E>
constexpr std::enable_if_t<EnableBitmaskOperators<E>::enable, E> operator&(E a, E b) {
	using U = std::underlying_type_t<E>;
	return static_cast<E>(static_cast<U>(a) & static_cast<U>(b));
}


template <typename E>
constexpr std::enable_if_t<EnableBitmaskOperators<E>::enable, E> operator~(E a) {
	using U = std::underlying_type_t<E>;
	return static_cast<E>(~static_cast<U>(a));
}


template <typename E>
constexpr std::enable_if_t<EnableBitmaskOperators<E>::enable, bool> HasFlag(E value, E flag) {
	return (value & flag) == flag;
}
