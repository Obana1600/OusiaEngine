#include "ObjParser.h"
#include "Core/Log.h"
#include "Core/Paths.h"

#include <fstream>
#include <sstream>
#include <string_view>
#include <unordered_map>

using namespace ou;


namespace {

int ResolveIndex(int value, size_t count) {
	if (value > 0) { return value - 1; }
	if (value < 0) { return static_cast<int>(count) + value; }
	return -1;
}

ObjData::Corner ParseCorner(std::string_view token, const ObjData &data) {
	int values[3] = {0, 0, 0};
	size_t begin = 0;

	for (int i = 0; i < 3; i++) {
		const auto end = token.find('/', begin);
		const auto part = token.substr(begin, end - begin);
		if (!part.empty()) {
			values[i] = std::stoi(std::string(part));
		}
		if (end == std::string::npos) {
			break;
		}
		begin = end + 1;
	}

	return {
		ResolveIndex(values[0], data.positions.size()),
		ResolveIndex(values[1], data.texcoords.size()),
		ResolveIndex(values[2], data.normals.size())};
}

void ParseMtlFile(const std::filesystem::path &path, std::vector<ObjData::Material> *materials) {
	std::ifstream file(path);
	if (!file) {
		Log::Warning("MTL ファイルを開けませんでした: {}", path.string());
		return;
	}

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream s(line);
		std::string key;
		s >> key;

		if (key == "newmtl") {
			materials->emplace_back();
			s >> materials->back().name;

		} else if (materials->empty()) {
			continue;

		} else if (key == "Kd") {
			auto &diffuse = materials->back().diffuse;
			s >> diffuse.x >> diffuse.y >> diffuse.z;

		} else if (key == "d") {
			s >> materials->back().alpha;

		} else if (key == "map_Kd") {
			std::string token;
			while (s >> token) {
				materials->back().diffuseMap = token;
			}
		}
	}
}

} // namespace



std::optional<ObjData> ou::ParseObjFile(const std::filesystem::path &path) {
	std::ifstream file(path);
	if (!file) {
		Log::Error("OBJ ファイルを開けませんでした: {}", path.string());
		return std::nullopt;
	}

	ObjData data;
	std::unordered_map<std::string, int> materialIndices;
	int currentMaterial = -1;

	const auto currentShape = [&]() -> ObjData::Shape & {
		if (data.shapes.empty()) {
			data.shapes.push_back({path.stem().string(), {}});
		}
		return data.shapes.back();
	};

	std::string line;
	std::vector<ObjData::Corner> polygon;
	while (std::getline(file, line)) {
		std::istringstream s(line);
		std::string key;
		s >> key;

		if (key == "v") {
			Vector3 position{};
			s >> position.x >> position.y >> position.z;
			data.positions.push_back(position);

		} else if (key == "vt") {
			Vector2 texcoord{};
			s >> texcoord.x >> texcoord.y;
			data.texcoords.push_back(texcoord);

		} else if (key == "vn") {
			Vector3 normal{};
			s >> normal.x >> normal.y >> normal.z;
			data.normals.push_back(normal);

		} else if (key == "o") {
			std::string name;
			s >> name;
			if (!data.shapes.empty() && data.shapes.back().triangles.empty()) {
				data.shapes.back().name = name;
			} else {
				data.shapes.push_back({name, {}});
			}

		} else if (key == "mtllib") {
			std::string fileName;
			s >> fileName;
			ParseMtlFile(path.parent_path() / fileName, &data.materials);
			for (size_t i = 0; i < data.materials.size(); i++) {
				materialIndices[data.materials[i].name] = static_cast<int>(i);
			}

		} else if (key == "usemtl") {
			std::string name;
			s >> name;
			const auto it = materialIndices.find(name);
			currentMaterial = (it != materialIndices.end()) ? it->second : -1;
			if (currentMaterial < 0) {
				Log::Warning("MTL ファイルにないマテリアルです: {}", name);
			}

		} else if (key == "f") {
			polygon.clear();
			std::string token;
			while (s >> token) {
				polygon.push_back(ParseCorner(token, data));
			}
			for (size_t i = 1; i + 1 < polygon.size(); i++) {
				currentShape().triangles.push_back({{polygon[0], polygon[i], polygon[i + 1]}, currentMaterial});
			}
		}
	}

	return data;
}
