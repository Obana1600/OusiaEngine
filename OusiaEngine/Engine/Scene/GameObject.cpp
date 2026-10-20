#include "GameObject.h"
#include "Diagnostics/Assert.h"

using namespace ou;


ou::GameObject::GameObject(Scene *scene, std::string_view name)
	: scene_(scene) {
	SetName(name);

	// Transform の追加
	auto transform = std::make_unique<Transform>();
	transform_ = transform.get();
	transform_->gameObject_ = this;
	transform_->transform_ = transform_;
	components_.push_back(std::move(transform));
}


ou::GameObject::~GameObject() {
	// Transform を最後に消す
	while (!components_.empty()) {
		components_.pop_back();
	}
}


Transform *ou::GameObject::GetTransform() const {
	return transform_;
}


Scene *ou::GameObject::GetScene() const {
	return scene_;
}


void ou::GameObject::StartComponents() {
	const auto count = components_.size();
	for (size_t i = 0; i < count; i++) {
		if (components_[i]->isStarted_ || components_[i]->IsDestroyed()) { continue; }
		components_[i]->isStarted_ = true;
		components_[i]->Start();
	}
}


void ou::GameObject::UpdateComponents() {
	const auto count = components_.size();
	for (size_t i = 0; i < count; i++) {
		if (!components_[i]->isStarted_ || components_[i]->IsDestroyed()) { continue; }
		components_[i]->Update();
	}
}


void ou::GameObject::LateUpdateComponents() {
	const auto count = components_.size();
	for (size_t i = 0; i < count; i++) {
		if (!components_[i]->isStarted_ || components_[i]->IsDestroyed()) { continue; }
		components_[i]->LateUpdate();
	}
}


void ou::GameObject::DestroyMarkedComponents() {
	ASSERT(!transform_->IsDestroyed(), "Transform は破棄できません。");

	for (const auto &comp : components_) {
		if (comp->IsDestroyed() && comp.get() != transform_) {
			comp->OnDestroy();
		}
	}

	std::erase_if(components_, [this](const auto &comp) {
		return comp->IsDestroyed() && comp.get() != transform_;
	});
}


void ou::GameObject::NotifyDestroyToAllComponents() {
	for (auto it = components_.rbegin(); it != components_.rend(); it++) {
		(*it)->OnDestroy();
	}
}

void ou::GameObject::ReplaceTransform(std::unique_ptr<Transform> newTransform) {
	auto *old = transform_;
	auto *next = newTransform.get();

	// コンポーネントとしての登録
	next->gameObject_ = this;
	next->transform_ = next;

	// SRT を引き継ぐ
	next->SetLocalPosition(old->GetLocalPosition());
	next->SetLocalRotation(old->GetLocalRotation());
	next->SetLocalScale(old->GetLocalScale());

	// 親子関係を引き継ぐ
	next->SetParent(old->GetParent());
	const auto children = old->GetChildren();
	for (auto *child : children) {
		child->SetParent(next);
	}
	old->SetParent(nullptr);

	// ほかのコンポーネントが持つ Transform を付け替える
	for (auto &comp : components_) {
		comp->transform_ = next;
	}

	// 新しい Transform に入れ替える
	ASSERT(components_.front().get() == old, "コンポーネントの先頭が Transform ではありません");
	transform_ = next;
	components_.front() = std::move(newTransform);

	next->Awake();
}
