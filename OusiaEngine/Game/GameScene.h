#pragma once

#include "Graphics/GraphicsDevice.h"
#include "Math/Color.h"
#include "Math/Vector3.h"
#include "Render/Asset/Shader.h"
#include "Render/Asset/Texture.h"
#include "Render/Renderer.h"
#include "Scene/Component.h"
#include "Scene/Scene.h"

#include <array>
#include <cstdint>
#include <memory>


namespace ou {

void BuildGameScene(Scene &scene);


class CameraController final: public Component {
public:
	void Update() override;
};


class InstancedPlanes final: public Renderer {
public:
	static constexpr uint32_t kInstanceCount = 10;

public:
	enum class BlendMode : int32_t {
		None,
		Normal,
		Add,
		Subtract,
		Multiply,
		Screen,
	};
	struct Plane {
		Vector3 translate{};
		float size = 2.0f;
		Color color = Color::White;
	};


public:
	void Awake() override;
	void Update() override;
	void Draw(Graphics::CommandList cmd) const override;

private:
	std::shared_ptr<Shader> shader_;
	std::shared_ptr<Texture> texture_;
	std::array<Plane, kInstanceCount> planes_{};
	BlendMode blendMode_ = BlendMode::None;
	Graphics::GpuBuffer instanceBuffers_[Graphics::GraphicsDevice::BUFFER_COUNT];

private:
	Shader::RenderStateOverride MakeRenderState() const;
};

} // namespace ou
