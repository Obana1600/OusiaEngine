#pragma once

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>


namespace ou {

template <typename... Args>
class Event {
public: // エイリアス

	using Listener = std::function<void(Args...)>;


public: // メンバ関数

	uint32_t AddListener(Listener listener);
	void RemoveListener(uint32_t id);

	void RemoveAllListeners();

	void Invoke(Args... args) const;


private: // インナークラス

	struct Entry {
		uint32_t id;
		Listener listener;
	};


private: // メンバ変数
	std::vector<Entry> listeners_;
	uint32_t nextId_ = 1;
};



template <typename... Args>
inline uint32_t Event<Args...>::AddListener(Listener listener) {
	const auto id = nextId_++;
	listeners_.push_back({id, std::move(listener)});
	return id;
}


template <typename... Args>
inline void Event<Args...>::RemoveListener(uint32_t id) {
	std::erase_if(listeners_, [id](const auto &entry) {
		return entry.id == id;
	});
}


template <typename... Args>
inline void Event<Args...>::RemoveAllListeners() {
	listeners_.clear();
}


template <typename... Args>
inline void Event<Args...>::Invoke(Args... args) const {
	const auto listeners = listeners_;
	for (const auto &entry : listeners) {
		entry.listener(args...);
	}
}

} // namespace ou
