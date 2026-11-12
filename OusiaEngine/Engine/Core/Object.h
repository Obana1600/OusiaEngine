#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>


namespace ou {

// 前方宣言
class Serializer;

class Object {
public: // 静的メンバ関数

	static void Destroy(Object *obj);

	static void DontDestroyOnLoad(Object *obj);

	static Object *FindObjectFromInstanceId(uint32_t instanceId);


public: // メンバ関数

	Object();
	Object(const Object &) = delete;
	Object &operator=(const Object &) = delete;
	virtual ~Object();

	virtual void Serialize(Serializer &) {}

	uint32_t GetInstanceId() const;
	bool IsDestroyed() const;

	void SetFileId(uint64_t fileId);
	uint64_t GetFileId() const;

	void SetName(std::string_view name);
	const std::string &GetName() const;


private: // 静的メンバ変数

	static inline std::unordered_map<uint32_t, Object *> sObjects_;
	static inline uint32_t sNextInstanceId_ = 1;


private: // メンバ変数

	std::string name_;
	uint32_t instanceId_ = 0;
	uint64_t fileId_ = 0;
	bool isDestroyed_ = false;
};

} // namespace ou
