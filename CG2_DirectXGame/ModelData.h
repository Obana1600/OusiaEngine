#pragma once

#include "VertexData.h"
#include "Vector.h"
#include <vector>
#include <string>


struct MaterialData {
	std::string textureFilePath;
	Vector3 diffuse; // 拡散光
	float opacity; // 不透明度
};


struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};


ModelData LoadObjFile(const std::string &directoryPath, const std::string &filename);

MaterialData LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename);