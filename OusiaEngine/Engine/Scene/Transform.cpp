#include "Transform.h"
#include "Diagnostics/Assert.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"
#include "Scene/GameObject.h"
#include "Serialize/Serializer.h"
#include "Scene/Scene.h"

#include <algorithm>
#include <limits>
#include <utility>

using namespace ou;


namespace {

template <typename T>
void MoveToIndex(std::vector<T *> &list, T *element, int index) {
	const auto it = std::find(list.begin(), list.end(), element);
	if (it == list.end()) { return; }
	list.erase(it);

	const auto clamped = std::clamp(index, 0, static_cast<int>(list.size()));
	list.insert(list.begin() + clamped, element);
}

} // namespace



ou::Transform::~Transform() {
	if (parent_) {
		std::erase(parent_->children_, this);
	}

	for (auto *child : children_) {
		child->parent_ = nullptr;
		child->MarkDirty();
	}
}


void ou::Transform::SetLocalPosition(const Vector3 &position) {
	localPosition_ = position;
	MarkDirty();
}


const Vector3 &ou::Transform::GetLocalPosition() const {
	return localPosition_;
}


void ou::Transform::SetLocalRotation(const Quaternion &rotation) {
	localRotation_ = rotation.Normalized();
	localEulerAnglesHint_ = localRotation_.EulerAngles();
	MarkDirty();
}


const Quaternion &ou::Transform::GetLocalRotation() const {
	return localRotation_;
}


void ou::Transform::SetLocalEulerAngles(const Vector3 &eulerAngles) {
	SetLocalRotation(Quaternion::Euler(eulerAngles));
	localEulerAnglesHint_ = eulerAngles;
}


Vector3 ou::Transform::GetLocalEulerAngles() const {
	return localRotation_.EulerAngles();
}


void ou::Transform::SetLocalScale(const Vector3 &scale) {
	localScale_ = scale;
	MarkDirty();
}


const Vector3 &ou::Transform::GetLocalScale() const {
	return localScale_;
}


const Matrix4x4 &ou::Transform::GetLocalToWorldMatrix() const {
	if (isDirty_) {
		localToWorld_ = Matrix4x4::SRT(localScale_, localRotation_, localPosition_);
		if (parent_) {
			localToWorld_ *= parent_->GetLocalToWorldMatrix();
		}
		isDirty_ = false;
	}

	return localToWorld_;
}


void ou::Transform::SetWorldPosition(const Vector3 &position) {
	SetLocalPosition(parent_ ? parent_->GetLocalToWorldMatrix().Inverse().MultiplyPoint3x4(position)
							 : position);
}


Vector3 ou::Transform::GetWorldPosition() const {
	const auto &m = GetLocalToWorldMatrix();
	return {m.m41, m.m42, m.m43};
}


void ou::Transform::SetWorldRotation(const Quaternion &rotation) {
	SetLocalRotation(parent_ ? Quaternion::Inverse(parent_->GetWorldRotation()) * rotation
							 : rotation);
}


Quaternion ou::Transform::GetWorldRotation() const {
	return parent_ ? parent_->GetWorldRotation() * localRotation_
				   : localRotation_;
}


Vector3 ou::Transform::GetLossyScale() const {
	const auto &m = GetLocalToWorldMatrix();
	return {Vector3(m.m11, m.m12, m.m13).Length(),
			Vector3(m.m21, m.m22, m.m23).Length(),
			Vector3(m.m31, m.m32, m.m33).Length()};
}


Vector3 ou::Transform::GetForward() const {
	const auto &m = GetLocalToWorldMatrix();
	return Vector3(m.m31, m.m32, m.m33).Normalized();
}


void ou::Transform::SetParent(Transform *parent) {
	if (parent == parent_) { return; }

	ASSERT(parent != this && (!parent || !parent->IsDescendantOf(this)),
		   "親子関係が循環します");

	auto *gameObject = GetGameObject();
	auto *scene = gameObject ? gameObject->GetScene() : nullptr;

	if (parent_) {
		std::erase(parent_->children_, this);
	} else if (scene) {
		std::erase(scene->rootGameObjects_, gameObject);
	}

	parent_ = parent;
	if (parent_) {
		parent_->children_.push_back(this);
	} else if (scene) {
		scene->rootGameObjects_.push_back(gameObject);
	}

	MarkDirty();
	OnTransformParentChanged();

	if (gameObject) {
		gameObject->RefreshActiveInHierarchy();
	}
}


void ou::Transform::SetParent(Transform *parent, bool worldPositionStays) {
	if (!worldPositionStays) {
		SetParent(parent);
		return;
	}

	const auto position = GetWorldPosition();
	const auto rotation = GetWorldRotation();
	const auto scale = GetLossyScale();

	SetParent(parent);
	SetWorldPosition(position);
	SetWorldRotation(rotation);

	const auto parentScale = parent ? parent->GetLossyScale() : Vector3::One;
	const auto divide = [](float value, float by, float current) {
		return (std::abs(by) > Mathf::Epsilon) ? (value / by) : current;
	};
	SetLocalScale({divide(scale.x, parentScale.x, localScale_.x),
				   divide(scale.y, parentScale.y, localScale_.y),
				   divide(scale.z, parentScale.z, localScale_.z)});
}


Transform *ou::Transform::GetParent() const {
	return parent_;
}


const std::vector<Transform *> &ou::Transform::GetChildren() const {
	return children_;
}


Transform *ou::Transform::Find(std::string_view path) {
	auto *current = this;

	while (!path.empty()) {
		const auto slash = path.find('/');
		const auto name = path.substr(0, slash);

		const auto &children = current->children_;
		const auto it = std::find_if(children.begin(), children.end(),
									 [&](const auto *child) { return child->GetGameObject()->GetName() == name; });
		if (it == children.end()) { return nullptr; }

		current = *it;
		path = (slash == std::string_view::npos) ? std::string_view{}
												 : path.substr(slash + 1);
	}

	return current;
}


void ou::Transform::SetSiblingIndex(int index) {
	if (parent_) {
		MoveToIndex(parent_->children_, this, index);
	} else {
		MoveToIndex(GetGameObject()->GetScene()->rootGameObjects_, GetGameObject(), index);
	}
}


int ou::Transform::GetSiblingIndex() const {
	if (parent_) {
		const auto &siblings = parent_->children_;
		return static_cast<int>(std::find(siblings.begin(), siblings.end(), this) - siblings.begin());
	}

	const auto &roots = GetGameObject()->GetScene()->rootGameObjects_;
	return static_cast<int>(std::find(roots.begin(), roots.end(), GetGameObject()) - roots.begin());
}


void ou::Transform::SetAsFirstSibling() {
	SetSiblingIndex(0);
}


void ou::Transform::SetAsLastSibling() {
	SetSiblingIndex(std::numeric_limits<int>::max());
}


void ou::Transform::Serialize(Serializer &serializer) {
	serializer.Field("localPosition", localPosition_);
	serializer.RotationField("localRotation", localRotation_, "localEulerAnglesHint", localEulerAnglesHint_);
	serializer.Field("localScale", localScale_);

	auto *parent = parent_;
	serializer.Reference("parent", parent);

	if (serializer.IsReading()) {
		constexpr float kHintTolerance = 1e-3f;
		if (Quaternion::Angle(Quaternion::Euler(localEulerAnglesHint_), localRotation_) > kHintTolerance) {
			localEulerAnglesHint_ = localRotation_.EulerAngles();
		}

		MarkDirty();

		if (parent == this || (parent && parent->IsDescendantOf(this))) {
			Log::Warning("親子関係が循環するため、親を参照できません: {}", GetGameObject()->GetName());
			parent = nullptr;
		}
		SetParent(parent);
	}
}


void ou::Transform::MarkDirty() {
	if (isDirty_) { return; }

	isDirty_ = true;
	for (auto *child : children_) {
		child->MarkDirty();
	}
}


bool ou::Transform::IsDescendantOf(const Transform *other) const {
	for (const auto *p = parent_; p; p = p->parent_) {
		if (p == other) {
			return true;
		}
	}

	return false;
}


void ou::Transform::TakeOverHierarchy(Transform *old) {
	parent_ = std::exchange(old->parent_, nullptr);
	if (parent_) {
		std::replace(parent_->children_.begin(), parent_->children_.end(), old, this);
	}

	children_ = std::move(old->children_);
	old->children_.clear();
	for (auto *child : children_) {
		child->parent_ = this;
		child->MarkDirty();
	}

	MarkDirty();
	OnTransformParentChanged();
}
