#pragma once

#include "Core/Object.h"
#include "Graphics/GraphicsTypes.h"

#include <deque>
#include <optional>
#include <string>
#include <vector>

// TODO: PipelineState を別の場所でキャッシュする

namespace ou {

class Shader final: public Object {
public: // インナークラス

	// TODO: 別ファイルに移す
	struct RenderQueue {
		static constexpr int BackGround = 1000;
		static constexpr int Geometry = 2000;
		static constexpr int AlphaTest = 2450;
		static constexpr int Transparent = 3000;
		static constexpr int Overlay = 4000;
	};

	struct Pass {
		std::string name;

		Graphics::Shader vertexShader;
		Graphics::Shader pixelShader;
		Graphics::Shader hullShader;
		Graphics::Shader domainShader;
		Graphics::Shader geometryShader;

		Graphics::BlendState blend;
		Graphics::RasterizerState rasterizer;
		Graphics::DepthStencilState depthStencil;
	};

	struct RenderStateOverride {
		std::optional<Graphics::Blend> srcBlend;
		std::optional<Graphics::Blend> dstBlend;
		std::optional<Graphics::BlendOp> blendOp;
		std::optional<bool> zWrite;
		std::optional<Graphics::CullMode> cull;

		bool operator==(const RenderStateOverride &) const = default;
	};


public: // メンバ関数

	Shader(std::vector<Pass> passes, int renderQueue);

	const Graphics::PipelineState *GetPipelineState(size_t passIndex,
													const RenderStateOverride &renderStateOverride,
													const Graphics::InputLayout *inputLayout,
													Graphics::PrimitiveTopology topology) const;

	const std::vector<Pass> &GetPasses() const;
	size_t GetPassCount() const;
	int GetRenderQueue() const;


private: // インナークラス

	struct CachedPipeline {
		RenderStateOverride renderStateOverride;
		const Graphics::InputLayout *inputLayout = nullptr;
		Graphics::PrimitiveTopology topology = Graphics::PrimitiveTopology::TRIANGLESTRIP;
		Graphics::PipelineState pipelineState;
	};


private: // メンバ変数

	std::vector<Pass> passes_;
	int renderQueue_ = RenderQueue::Geometry;

	mutable std::vector<std::deque<CachedPipeline>> pipelineCaches_;
};

} // namespace ou
