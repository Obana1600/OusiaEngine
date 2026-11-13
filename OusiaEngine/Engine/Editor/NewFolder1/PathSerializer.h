#pragma once

#include "Serialize/Serializer.h"

#include <string>
#include <string_view>
#include <vector>


namespace ou {

class PathSerializer: public Serializer {
protected: // メンバ関数 

	std::string MakePath(std::string_view name);

	void PushFrame(std::string path, bool isArray);
	void PopFrame();


private: // インナークラス

	struct Frame {
		std::string path;
		bool isArray = false;
		size_t nextIndex = 0;
	};


private: // メンバ変数

	std::vector<Frame> frames_;
};

} // namespace ou
