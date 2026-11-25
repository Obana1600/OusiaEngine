#pragma once

#include "Scene/Behavior.h"

#include <memory>


namespace ou {

// 前方宣言
class VolumeProfile;

class Volume final: public Behavior {
public: // メンバ関数

	void OnEnable() override;
	void OnDisable() override;
	void Serialize(Serializer &serializer) override;

	void SetIsGlobal(bool isGlobal);
	bool GetIsGlobal() const;

	void SetPriority(float priority);
	float GetPriority() const;

	void SetWeight(float weight);
	float GetWeight() const;

	void SetBlendDistance(float blendDistance);
	float GetBlendDistance() const;

	void SetSharedProfile(std::shared_ptr<VolumeProfile> sharedProfile);
	const std::shared_ptr<VolumeProfile> &GetSharedProfile() const;


private: // メンバ変数

	bool isGlobal_ = true;
	float priority_ = 0.0f;
	float weight_ = 1.0f;
	float blendDistance_ = 0.0f;

	std::shared_ptr<VolumeProfile> sharedProfile_;
};

} // namespace ou
