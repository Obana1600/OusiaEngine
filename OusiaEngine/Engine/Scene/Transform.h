#pragma once

#include "Component.h"
#include "Math/Matrix4x4.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"

#include <string_view>
#include <vector>

// TODO: API 名を Unity にそろえる

namespace ou {

class Transform: public Component {
	friend class GameObject;

public: // メンバ関数

	~Transform() override;

	void SetLocalPosition(const Vector3 &position);
	const Vector3 &GetLocalPosition() const;

	void SetLocalRotation(const Quaternion &rotation);
	const Quaternion &GetLocalRotation() const;

	void SetLocalEulerAngles(const Vector3 &eulerAngles);
	Vector3 GetLocalEulerAngles() const;

	void SetLocalScale(const Vector3 &scale);
	const Vector3 &GetLocalScale() const;

	const Matrix4x4 &GetLocalToWorldMatrix() const;

	void SetWorldPosition(const Vector3 &position);
	Vector3 GetWorldPosition() const;

	void SetWorldRotation(const Quaternion &rotation);
	Quaternion GetWorldRotation() const;

	Vector3 GetLossyScale() const;
	Vector3 GetForward() const;

	void SetParent(Transform *parent);
	void SetParent(Transform *parent, bool worldPositionStays);
	Transform *GetParent() const;
	const std::vector<Transform *> &GetChildren() const;
	Transform *Find(std::string_view path);

	void SetSiblingIndex(int index);
	int GetSiblingIndex() const;
	void SetAsFirstSibling();
	void SetAsLastSibling();

	void Serialize(Serializer &serializer) override;


protected: // メンバ関数

	virtual void OnTransformParentChanged() {};


private: // メンバ変数

	Vector3 localPosition_ = Vector3::Zero;
	Quaternion localRotation_ = Quaternion::Identity;
	Vector3 localEulerAnglesHint_ = Vector3::Zero;
	Vector3 localScale_ = Vector3::One;

	Transform *parent_ = nullptr;
	std::vector<Transform *> children_;

	mutable Matrix4x4 localToWorld_ = Matrix4x4::Identity;
	mutable bool isDirty_ = true;


private: // メンバ関数

	void MarkDirty();

	bool IsDescendantOf(const Transform *other) const;

	void TakeOverHierarchy(Transform *old);
};

} // namespace ou
