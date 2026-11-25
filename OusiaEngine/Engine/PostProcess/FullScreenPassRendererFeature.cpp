#include "FullScreenPassRendererFeature.h"
#include "Render/Asset/Material.h"
#include "Serialize/Serializer.h"

using namespace ou;


void ou::FullScreenPassRendererFeature::Serialize(Serializer &serializer) {
	serializer.Field("name", name_);
	serializer.Field("active", isActive_);
	serializer.Asset("passMaterial", passMaterial_);
	serializer.Enum("injectionPoint", injectionPoint_);
	serializer.Field("fetchColorBuffer", fetchColorBuffer_);
	serializer.Flags("requirements", requirements_);
}


void ou::FullScreenPassRendererFeature::SetName(std::string_view name) {
	name_ = name;
}


const std::string &ou::FullScreenPassRendererFeature::GetName() const {
	return name_;
}


void ou::FullScreenPassRendererFeature::SetActive(bool active) {
	isActive_ = active;
}


bool ou::FullScreenPassRendererFeature::GetIsActive() const {
	return isActive_;
}
