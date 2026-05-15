#include "ModelData.h"
#include "Vector.h"
#include <fstream>
#include <sstream>
#include <cassert>


ModelData LoadObjFile(const std::string &directoryPath, const std::string &filename) {
	// 必要となる変数
	ModelData modelData{}; // 構築するModelData
	std::vector<Vector4> positions{}; // 位置
	std::vector<Vector3> normals{}; // 法線
	std::vector<Vector2> texcoords{}; // テクスチャ座標
	std::string line{}; // ファイルから読んだ1行を格納するもの

	// ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	// テキストファイルを1行ずつ読む
	while (std::getline(file, line)) {
		std::string identifier{};
		std::istringstream s(line);
		s >> identifier; // 先頭の識別子を読む

		if (identifier == "v") {
			Vector4 position{};
			s >> position.x >> position.y >> position.z;
			position.x *= -1.0f; // x軸反転
			position.w = 1.0f;
			positions.push_back(position);
		} else if (identifier == "vt") {
			Vector2 texcoord{};
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y; // uv上下反転
			texcoords.push_back(texcoord);
		} else if (identifier == "vn") {
			Vector3 normal{};
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1.0f; // x軸反転
			normals.push_back(normal);
		} else if (identifier == "f") {
			// 面は三角形限定。その他は未対応
			VertexData triangle[3]{};
			for (int32_t faceVertex = 0; faceVertex < 3; faceVertex++) {
				std::string vertexDefinition{};
				s >> vertexDefinition;
				// 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3]{};
				for (int32_t element = 0; element < 3; element++) {
					std::string index{};
					std::getline(v, index, '/'); // /区切りでインデックスを読んでいく
					elementIndices[element] = std::stoi(index);
				}
				// 要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				triangle[faceVertex] = { position, texcoord, normal };
			}
			// 頂点を逆順で登録することで、回り順を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		} else if (identifier == "mtllib") {
			// materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename{};
			s >> materialFilename;
			// 基本的にobjファイルを同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}

	return modelData;
}


MaterialData LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename) {
	// 必要となる変数
	MaterialData materialData{}; // 構築するMaterialData
	std::string line{}; // ファイルから読んだ1行を格納するもの

	// ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string identifier{};
		std::istringstream s(line);
		s >> identifier;

		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename{};
			s >> textureFilename;
			// 連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		} else if (identifier == "Kd") {
			s >> materialData.diffuse.x >> materialData.diffuse.y >> materialData.diffuse.z;
		} else if (identifier == "d") {
			s >> materialData.opacity;
		}
	}

	return materialData;
}