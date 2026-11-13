#include "PathSerializer.h"


std::string ou::PathSerializer::MakePath(std::string_view name) {
	std::string segment;
	if (!frames_.empty() && frames_.back().isArray) {
		segment = std::to_string(frames_.back().nextIndex++);
	} else {
		segment = std::string(name);
	}
	return frames_.empty() ? segment : frames_.back().path + "/" + segment;
}


void ou::PathSerializer::PushFrame(std::string path, bool isArray) {
	frames_.push_back({std::move(path), isArray, 0});
}


void ou::PathSerializer::PopFrame() {
	if (!frames_.empty()) { frames_.pop_back(); }
}
