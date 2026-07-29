#include "GameScene.h"

#include "3d/Model.h"
#include "base/TextureManager.h"
#include "externals/imgui/imgui.h"
#include "math/MathUtility.h"
#include <cassert>
#include <numbers>

using namespace MathUtility;


namespace {

	static const char *labels[] = {
		"平面",
		"スプライト",
		"球",
		"ティーポット",
		"ウサギ",
		"複数メッシュ",
		"複数マテリアル",
		"スザンヌ"
	};

	static const char *assets[] = {
		"plane",
		"sprite",
		"sphere",
		"teapot",
		"bunny",
		"multiMesh",
		"multiMaterial",
		"suzanne"
	};

	const char *ToLabel(ObjectType name) {
		return labels[static_cast<int>(name)];
	}

	const char *ToAsset(ObjectType name) {
		return assets[static_cast<int>(name)];
	}

} // namespace



void GameScene::Initialize() {
	camera_.Initialize();

	uvCheckerHandle = TextureManager::Load("uvChecker.png");

	CreateObject(ObjectType::Plane);
	auto it = objects_.begin();
	it->second.worldTransform.rotation_.y = std::numbers::pi_v<float>;
}


void GameScene::Update() {

	OnImGui();

	camera_.Update();

	for (auto &[name, obj] : objects_) {
		obj.worldTransform.Update();
	}
}


void GameScene::Draw() const {
	Model::PreDraw();

	for (auto &[name, obj] : objects_) {
		obj.model->Draw(obj.worldTransform, camera_.GetCamera());
	}

	Model::PostDraw();


	Sprite::PreDraw();

	for (auto &sprite : sprites_) {
		sprite->Draw();
	}

	Sprite::PostDraw();
}


void GameScene::OnImGui() {
	ImGui::Begin("設定");

	// オブジェクト生成 ------------------------------------------------------------------------------

	static int currentName = 0;
	ImGui::Combo("モデル", &currentName, labels, IM_ARRAYSIZE(labels));

	if (ImGui::Button("生成")) {
		CreateObject(static_cast<ObjectType>(currentName));
	}

	// --------------------------------------------------------------------------------------------


	if (!objects_.empty()) {
		ImGui::Separator();
	}


	// model -----------------------------------------------------------------------------------

	std::string deleteName;
	for (auto &[name, obj] : objects_) {
		if (!ImGui::CollapsingHeader(name.c_str())) {
			continue;
		}
		ImGui::PushID(name.c_str());

		// transform
		ImGui::DragFloat3("座標", &obj.worldTransform.translation_.x, 0.01f);
		ImGui::DragFloat3("回転", &obj.worldTransform.rotation_.x, 0.01f);
		ImGui::DragFloat3("スケール", &obj.worldTransform.scale_.x, 0.01f);

		if (ImGui::Button("削除")) {
			deleteName = name;
		}

		ImGui::Indent();

		// material
		for (auto &[mtlName, mtl] : obj.model->GetMaterials()) {
			if (!ImGui::CollapsingHeader(mtlName.c_str())) {
				continue;
			}
			ImGui::PushID(mtlName.c_str());

			// uvTransform
			ImGui::DragFloat2("UV座標", &mtl->uvOffset_.x, 0.01f);
			ImGui::DragFloat("UV回転", &mtl->uvRotate_, 0.01f);
			ImGui::DragFloat2("UVスケール", &mtl->uvScale_.x, 0.01f);

			// color
			ImGui::ColorEdit3("色", &mtl->color_.x);
			ImGui::DragFloat("不透明度", &mtl->alpha_, 0.001f, 0.0f, 1.0f);

			int lightMode = mtl->lightMode_;
			if (ImGui::Combo("ライティング方式", &lightMode, "なし\0Lambert\0HalfLambert\0")) {
				mtl->lightMode_ = lightMode;
			}

			ImGui::PopID();
		}

		ImGui::Unindent();

		ImGui::PopID();
	}

	// delete
	if (!deleteName.empty()) {
		objects_.erase(deleteName);
		objCounter_--;
	}

	// --------------------------------------------------------------------------------------------


	if (!sprites_.empty()) {
		ImGui::Separator();
	}


	// sprite -------------------------------------------------------------------------------------

	int deleteIndex = -1;
	for (int i = 0; i < static_cast<int>(sprites_.size()); i++) {
		auto key = "スプライト" + std::to_string(i);
		if (!ImGui::CollapsingHeader(key.c_str())) {
			continue;
		}
		ImGui::PushID(key.c_str());

		// transform
		auto translation = sprites_[i]->GetTranslation();
		if (ImGui::DragFloat2("座標", &translation.x, 0.01f)) {
			sprites_[i]->SetTranslation(translation);
		}
		auto rotation = sprites_[i]->GetRotation();
		if (ImGui::DragFloat3("回転", &rotation.x, 0.01f)) {
			sprites_[i]->SetRotation(rotation);
		}
		auto scale = sprites_[i]->GetScale();
		if (ImGui::DragFloat2("スケール", &scale.x, 0.01f)) {
			sprites_[i]->SetScale(scale);
		}

		if (ImGui::Button("削除")) {
			deleteIndex = i;
		}

		// material
		ImGui::Indent();
		if (ImGui::CollapsingHeader("Material: uvChecker.png")) {
			// uvTransform
			auto uvTranslation = sprites_[i]->GetUVTranslation();
			if (ImGui::DragFloat2("UV座標", &uvTranslation.x, 0.01f)) {
				sprites_[i]->SetUVTranslation(uvTranslation);
			}
			auto uvRotation = sprites_[i]->GetUVRotation();
			if (ImGui::DragFloat("UV回転", &uvRotation, 0.01f)) {
				sprites_[i]->SetUVRotation(uvRotation);
			}
			auto uvScale = sprites_[i]->GetUVScale();
			if (ImGui::DragFloat2("UVスケール", &uvScale.x, 0.01f)) {
				sprites_[i]->SetUVScale(uvScale);
			}

			// color
			auto color = sprites_[i]->GetColor();
			if (ImGui::ColorEdit4("色", &color.x)) {
				sprites_[i]->SetColor(color);
			}
		}
		ImGui::Unindent();

		ImGui::PopID();
	}

	// delete
	if (deleteIndex >= 0) {
		sprites_.erase(sprites_.begin() + deleteIndex);
	}

	// --------------------------------------------------------------------------------------------


	ImGui::Separator();


	// light --------------------------------------------------------------------------------------

	if (ImGui::CollapsingHeader("平行光源")) {
		auto &light = ModelCommon::GetInstance()->GetDirectionLight();

		ImGui::ColorEdit4("色", &light.color_.x);

		static auto &direction = light.direction_;
		ImGui::SliderFloat3("方向", &direction.x, -1.0f, 1.0f);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			direction = Normalize(direction);
			light.direction_ = direction;
		}

		ImGui::DragFloat("輝度", &light.intensity_, 0.01f);
	}

	// --------------------------------------------------------------------------------------------

	ImGui::End();
}


void GameScene::CreateObject(ObjectType type) {
	if (type == ObjectType::Sprite) {
		sprites_.push_back(std::unique_ptr<Sprite>(Sprite::Create(uvCheckerHandle, {0.0f, 0.0f})));
		return;
	}

	auto *modelName = ToAsset(type);
	std::string key = ToLabel(type) + std::to_string(objCounter_);

	// 空きスロットを作ってから差し込む
	auto [it, ok] = objects_.try_emplace(key);
	it->second.model = std::unique_ptr<Model>(Model::CreateFromObj(modelName));
	it->second.worldTransform.Initialize();

	objCounter_++;
}