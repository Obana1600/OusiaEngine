#pragma once

#include "Math/Vector2.h"
#include "Math/Vector3.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// TODO: gITF に対応させたい

namespace ou {

struct ObjData {

	struct Corner {
		int position = -1;
		int texcoord = -1;
		int normal = -1;
	};

	struct Triangle {
		Corner corners[3];
		int material = -1;
	};

	struct Shape {
		std::string name;
		std::vector<Triangle> triangles;
	};

	struct Material {
		std::string name;
		Vector3 diffuse = Vector3::One;
		float alpha = 1.0f;
		std::string diffuseMap;
	};

	std::vector<Vector3> positions;
	std::vector<Vector2> texcoords;
	std::vector<Vector3> normals;
	std::vector<Shape> shapes;
	std::vector<Material> materials;
};

std::optional<ObjData> ParseObjFile(const std::filesystem::path &path);

} // namespace ou
