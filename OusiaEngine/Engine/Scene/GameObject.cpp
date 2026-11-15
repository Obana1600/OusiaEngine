#include "GameObject.h"
#include "Diagnostics/Assert.h"
#include "Diagnostics/Log.h"
#include "Scene/Behavior.h"
#include "Scene/LayerMask.h"
#include "Serialize/Serializer.h"


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


void ou::GameObject::Serialize(Serializer &serializer) {
	auto name = GetName();
	serializer.Field("name", name);
	if (serializer.IsReading()) { SetName(name); }

	serializer.Field("active", isActiveSelf_);

	auto layer = layer_;
	serializer.Field("layer", layer);
	if (serializer.IsReading()) { SetLayer(layer); }
}


Transform *ou::GameObject::GetTransform() const {
	return transform_;
}


Scene *ou::GameObject::GetScene() const {
	return scene_;
}


void ou::GameObject::SetActive(bool value) {
	if (isActiveSelf_ == value) { return; }

	isActiveSelf_ = value;
	RefreshActiveInHierarchy();
}


bool ou::GameObject::IsActiveSelf() const {
	return isActiveSelf_;
}


bool ou::GameObject::IsActiveInHierarchy() const {
	if (!isActiveSelf_) { return false; }

	const auto *parent = transform_->GetParent();
	return !parent || parent->GetGameObject()->IsActiveInHierarchy();
}


void ou::GameObject::SetLayer(int layer) {
	if (layer < 0 || layer >= LayerMask::kLayerCount) {
		Log::Error("レイヤーは 0~31 で指定してください: {} ({})", layer, GetName());
		return;
	}
	layer_ = layer;
}


int ou::GameObject::GetLayer() const {
	return layer_;
}


Component *ou::GameObject::AttachComponent(std::unique_ptr<Component> component) {
	auto *ptr = component.get();
	ptr->gameObject_ = this;
	ptr->transform_ = transform_;
	components_.push_back(std::move(component));
	return ptr;
}


Component *ou::GameObject::AddComponentInstance(std::unique_ptr<Component> component) {
	if (auto *transform = dynamic_cast<Transform *>(component.get())) {
		component.release();
		ReplaceTransform(std::unique_ptr<Transform>(transform));
		return transform;
	}

	auto *added = AttachComponent(std::move(component));
	if (IsActiveInHierarchy()) {
		added->isAwoken_ = true;
		added->Awake();
		RefreshBehavior(added);
	}
	return added;
}


void ou::GameObject::AwakeComponents() {
	const auto count = components_.size();
	for (size_t i = 0; i < count; i++) {
		if (components_[i]->isAwoken_) { continue; }

		components_[i]->isAwoken_ = true;
		components_[i]->Awake();
	}
}


void ou::GameObject::StartComponents() {
	if (!IsActiveInHierarchy()) { return; }

	const auto count = components_.size();
	for (size_t i = 0; i < count; i++) {
		if (components_[i]->isStarted_ ||
			components_[i]->IsDestroyed() ||
			!components_[i]->IsEnabledSelf()) { continue; }
		components_[i]->isStarted_ = true;
		components_[i]->Start();
	}
}


void ou::GameObject::FixedUpdateComponents() {
	if (!IsActiveInHierarchy()) { return; }

	const auto count = components_.size();
	for (size_t i = 0; i < count; i++) {
		if (!components_[i]->isStarted_ ||
			components_[i]->IsDestroyed() ||
			!components_[i]->IsEnabledSelf()) { continue; }
		components_[i]->FixedUpdate();
	}
}


void ou::GameObject::UpdateComponents() {
	if (!IsActiveInHierarchy()) { return; }

	const auto count = components_.size();
	for (size_t i = 0; i < count; i++) {
		if (!components_[i]->isStarted_ ||
			components_[i]->IsDestroyed() ||
			!components_[i]->IsEnabledSelf()) { continue; }
		components_[i]->Update();
	}
}


void ou::GameObject::LateUpdateComponents() {
	if (!IsActiveInHierarchy()) { return; }

	const auto count = components_.size();
	for (size_t i = 0; i < count; i++) {
		if (!components_[i]->isStarted_ ||
			components_[i]->IsDestroyed() ||
			!components_[i]->IsEnabledSelf()) { continue; }
		components_[i]->LateUpdate();
	}
}


void ou::GameObject::DestroyMarkedComponents() {
	ASSERT(!transform_->IsDestroyed(), "Transform は破棄できません。");

	for (const auto &comp : components_) {
		if (comp->IsDestroyed() && comp.get() != transform_) {
			if (comp->isAwoken_) {
				DisableBeheviorForDestroy(comp.get());
				comp->OnDestroy();
			}
		}
	}

	std::erase_if(components_, [this](const auto &comp) {
		return comp->IsDestroyed() && comp.get() != transform_;
	});
}


void ou::GameObject::NotifyDestroyToAllComponents() {
	for (auto it = components_.rbegin(); it != components_.rend(); it++) {
		if ((*it)->isAwoken_) {
			DisableBeheviorForDestroy((*it).get());
			(*it)->OnDestroy();
		}
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
	next->TakeOverHierarchy(old);

	// ほかのコンポーネントが持つ Transform を付け替える
	for (auto &comp : components_) {
		comp->transform_ = next;
	}

	// 新しい Transform に入れ替える
	ASSERT(components_.front().get() == old, "コンポーネントの先頭が Transform ではありません");
	transform_ = next;
	components_.front() = std::move(newTransform);

	if (IsActiveInHierarchy()) {
		next->isAwoken_ = true;
		next->Awake();
	}
}


void ou::GameObject::RefreshActiveInHierarchy() {
	if (IsActiveInHierarchy()) { AwakeComponents(); }

	for (size_t i = 0; i < components_.size(); i++) {
		RefreshBehavior(components_[i].get());
	}

	const auto children = transform_->GetChildren();
	for (auto *child : children) {
		child->GetGameObject()->RefreshActiveInHierarchy();
	}
}


void ou::GameObject::RefreshBehavior(Component *component) {
	if (auto *behavior = dynamic_cast<Behavior *>(component)) {
		behavior->RefreshActiveAndEnabled();
	}
}


void ou::GameObject::DisableBeheviorForDestroy(Component *component) {
	if (auto *behavior = dynamic_cast<Behavior *>(component)) {
		behavior->DisableForDestroy();
	}
}
