#pragma once

#include "Component.h"
#include "Math/Matrix4x4.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"

#include <vector>

// TODO: API 名を Unity にそろえる

namespace ou {

class Transform final: public Component {
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
	Vector3 GetWorldPosition() const;
	Vector3 GetForward() const;

	Transform *GetParent() const;
	void SetParent(Transform *parent);
	const std::vector<Transform *> &GetChildren() const;


private: // メンバ変数

	Vector3 localPosition_{};
	Quaternion localRotation_ = Quaternion::Identity;
	Vector3 localScale_ = Vector3::One;

	Transform *parent_ = nullptr;
	std::vector<Transform *> children_;

	mutable Matrix4x4 localToWorld_ = Matrix4x4::Identity;
	mutable bool isDirty_ = true;


private: // メンバ関数

	void MarkDirty();

	bool IsDescendantOf(const Transform *other) const;
};

} // namespace ou
