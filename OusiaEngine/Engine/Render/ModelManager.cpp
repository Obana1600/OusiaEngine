#include "ModelManager.h"
#include "Render/Model.h"

using namespace ou;


ModelManager *ou::ModelManager::GetInstance() {
	static ModelManager instance;
	return &instance;
}


std::shared_ptr<const Model> ou::ModelManager::Load(std::string_view fileName) {
	auto &models = GetInstance()->models_;

	const std::string key(fileName);
	if (const auto it = models.find(key); it != models.end()) {
		return it->second;
	}

	auto model = Model::LoadObj(fileName);
	if (model) {
		models.emplace(key, model);
	}
	return model;
}


void ou::ModelManager::Finalize() {
	models_.clear();
}
