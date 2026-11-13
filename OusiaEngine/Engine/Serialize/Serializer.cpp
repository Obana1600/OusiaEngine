#include "Serializer.h"

using namespace ou;


void ou::Serializer::RotationField(std::string_view name, Quaternion &rotation, std::string_view hintName, Vector3 &eulerHint) {
	Field(name, rotation);
	Field(hintName, eulerHint);
}


void ou::Serializer::EnumField(std::string_view name, std::string &text, std::span<const std::string_view>) {
	Field(name, text);
}
