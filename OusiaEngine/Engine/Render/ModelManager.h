#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>


namespace ou {

// 前方宣言
class Model;

class ModelManager {
public: // 静的メンバ関数

	static ModelManager *GetInstance();

	static std::shared_ptr<const Model> Load(std::string_view fileName);


public: // メンバ関数

	void Finalize();


private: // メンバ変数

	std::unordered_map<std::string, std::shared_ptr<const Model>> models_;


private: // コンストラクタ

	ModelManager() = default;
	~ModelManager() = default;
	ModelManager(const ModelManager &) = delete;
	ModelManager &operator=(const ModelManager &) = delete;
};

} // namespace ou
