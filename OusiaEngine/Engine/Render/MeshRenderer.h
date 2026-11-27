#pragma once

#include "Render/Renderer.h"

#include <memory>


namespace ou {

// 前方宣言
class Mesh;

class MeshRenderer final: public Renderer {
public: // メンバ関数

	void Serialize(Serializer &serializer) override;

	void Draw(std::string_view lightMode, Graphics::CommandList cmd) const override;

	void SetMesh(std::shared_ptr<Mesh> mesh);
	const std::shared_ptr<Mesh> &GetMesh() const;


private: // メンバ変数

	std::shared_ptr<Mesh> mesh_;
};

} // namespace ou
