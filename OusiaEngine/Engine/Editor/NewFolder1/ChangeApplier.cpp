#include "ChangeApplier.h"

using namespace ou;


ou::ChangeApplier::ChangeApplier(const FieldChange &change)
	: change_(change) {}


bool ou::ChangeApplier::IsReading() const {
	return true;
}


void ou::ChangeApplier::Field(std::string_view name, bool &value) {
	Apply(name, value);
}


void ou::ChangeApplier::Field(std::string_view name, int32_t &value) {
	Apply(name, value);
}


void ou::ChangeApplier::Field(std::string_view name, uint32_t &value) {
	Apply(name, value);
}


void ou::ChangeApplier::Field(std::string_view name, float &value) {
	Apply(name, value);
}


void ou::ChangeApplier::Field(std::string_view name, Vector2 &value) {
	Apply(name, value);
}


void ou::ChangeApplier::Field(std::string_view name, Vector3 &value) {
	Apply(name, value);
}


void ou::ChangeApplier::Field(std::string_view name, Vector4 &value) {
	Apply(name, value);
}


void ou::ChangeApplier::Field(std::string_view name, Quaternion &value) {
	Apply(name, value);
}


void ou::ChangeApplier::Field(std::string_view name, Color &value) {
	Apply(name, value);
}


void ou::ChangeApplier::Field(std::string_view name, Rect &value) {
	Apply(name, value);
}


void ou::ChangeApplier::Field(std::string_view name, std::string &value) {
	Apply(name, value);
}


void ou::ChangeApplier::RotationField(std::string_view name, Quaternion &rotation, std::string_view, Vector3 &eulerHint) {
	if (MakePath(name) != change_.path) { return; }
	if (const auto *hint = std::get_if<Vector3>(&change_.value)) {
		eulerHint = *hint;
		rotation = Quaternion::Euler(*hint);
	}
}


bool ou::ChangeApplier::BeginObject(std::string_view name) {
	PushFrame(MakePath(name), false);
	return true;
}


void ou::ChangeApplier::EndObject() {
	PopFrame();
}


bool ou::ChangeApplier::BeginArray(std::string_view name, size_t &count) {
	const auto path = MakePath(name);
	if (path == change_.path) {
		if (const auto *size = std::get_if<size_t>(&change_.value)) { count = *size; }
	}

	PushFrame(path, true);
	return true;
}


void ou::ChangeApplier::EndArray() {
	PopFrame();
}


void ou::ChangeApplier::ObjectReference(std::string_view name, Object *&value, ObjectFilter) {
	Apply(name, value);
}


void ou::ChangeApplier::AssetObject(std::string_view name, std::shared_ptr<Object> &value, const AssetType &) {
	Apply(name, value);
}


void ou::ChangeApplier::EnumField(std::string_view name, std::string &text, std::span<const std::string_view>) {
	Apply(name, text);
}


void ou::ChangeApplier::FlagsField(std::string_view name, std::string &text, std::span<const std::string_view>) {
	Apply(name, text);
}
