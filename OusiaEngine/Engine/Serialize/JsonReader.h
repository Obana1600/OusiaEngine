#pragma once

#include "Core/JsonFile.h"
#include "Serialize/Serializer.h"

#include <cstddef>
#include <unordered_map>
#include <vector>


namespace ou {

class JsonReader final: public Serializer {
public: // メンバ関数

	JsonReader(const Json &source, const std::unordered_map<uint64_t, Object *> &objectsById);

	bool IsReading() const override;

	void Field(std::string_view name, bool &value) override;
	void Field(std::string_view name, int32_t &value) override;
	void Field(std::string_view name, uint32_t &value) override;
	void Field(std::string_view name, float &value) override;
	void Field(std::string_view name, Vector2 &value) override;
	void Field(std::string_view name, Vector3 &value) override;
	void Field(std::string_view name, Vector4 &value) override;
	void Field(std::string_view name, Quaternion &value) override;
	void Field(std::string_view name, Color &value) override;
	void Field(std::string_view name, Rect &value) override;
	void Field(std::string_view name, std::string &value) override;

	bool BeginObject(std::string_view name) override;
	void EndObject() override;
	bool BeginArray(std::string_view name, size_t &count) override;
	void EndArray() override;

	const Json *FindReadingJson() const override;


private: // インナークラス

	struct Frame {
		const Json *node = nullptr;
		size_t next = 0; // 配列のとき、次に読む要素の番号
	};


private: // メンバ変数

	std::vector<Frame> stack_;
	const std::unordered_map<uint64_t, Object *> *objectsById_ = nullptr;


private: // メンバ関数

	void ObjectReference(std::string_view name, Object *&value, ObjectFilter) override;
	void AssetObject(std::string_view name, std::shared_ptr<Object> &value, const AssetType &) override;

	const Json *FindValue(std::string_view name);

	bool ReadFloats(std::string_view name, float *out, size_t count);
};

} // namespace ou
