#include "Serializer.h"
#include "Core/JsonFile.h"

using namespace ou;


void ou::Serializer::RotationField(std::string_view name, Quaternion &rotation, std::string_view hintName, Vector3 &eulerHint) {
	Field(name, rotation);
	Field(hintName, eulerHint);
}


const Json *ou::Serializer::FindReadingJson() const {
	return nullptr;
}


Json *ou::Serializer::FindWritingJson() {
	return nullptr;
}


void ou::Serializer::EnumField(std::string_view name, std::string &text, std::span<const std::string_view>) {
	Field(name, text);
}


void ou::Serializer::FlagsField(std::string_view name, std::string &text, std::span<const std::string_view>) {
	Field(name, text);
}
