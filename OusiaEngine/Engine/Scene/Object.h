#pragma once

#include <cstdint>
#include <string>
#include <string_view>


namespace ou {

class Object {
public: // 静的メンバ関数

	static void Destroy(Object *obj);


public: // メンバ関数

	Object();
	Object(const Object &) = delete;
	Object &operator=(const Object &) = delete;

	virtual ~Object() = default;

	uint32_t GetInstanceId() const;
	bool IsDestroyed() const;

	void SetName(std::string_view name);
	const std::string &GetName() const;


private: // 静的メンバ変数

	static inline uint32_t sNextInstanceId_ = 1;


private: // メンバ変数

	std::string name_;
	uint32_t instanceId_ = 0;
	bool isDestroyed_ = false;
};

} // namespace ou
