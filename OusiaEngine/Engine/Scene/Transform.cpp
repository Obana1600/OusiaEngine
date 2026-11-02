#include "Transform.h"
#include "Diagnostics/Assert.h"
#include "Scene/GameObject.h"

#include <algorithm>

using namespace ou;


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
	MarkDirty();
}


const Quaternion &ou::Transform::GetLocalRotation() const {
	return localRotation_;
}


void ou::Transform::SetLocalEulerAngles(const Vector3 &eulerAngles) {
	SetLocalRotation(Quaternion::Euler(eulerAngles));
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


Transform *ou::Transform::GetParent() const {
	return parent_;
}


void ou::Transform::SetParent(Transform *parent) {
	if (parent == parent_) { return; }

	ASSERT(parent != this && (!parent || !parent->IsDescendantOf(this)),
		   "親子関係が循環します");

	// すでに親がいるなら、自身を親の子から消す
	if (parent_) {
		auto &siblings = parent_->children_;
		siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
	}

	parent_ = parent;
	if (parent_) { parent_->children_.push_back(this); }
	MarkDirty();
	OnTransformParentChanged();

	if (auto *gameObject = GetGameObject()) {
		gameObject->RefreshActiveInHierarchy();
	}
}


const std::vector<Transform *> &ou::Transform::GetChildren() const {
	return children_;
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
