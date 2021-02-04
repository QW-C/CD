#pragma once

#include <CD/Graphics/Common.hpp>
#include <CD/Graphics/Scene.hpp>

namespace CD {

class Frame;

class Sky {
public:
	Sky(Frame&);
	~Sky();

	void set_texture(const Texture*);

	void render(GPU::CommandBuffer&, const Camera&);
private:
	Frame& frame;

	const Texture* texture;
	const GraphicsPipeline* graphics_pipeline;
	GPU::PipelineHandle skybox;

	struct SkyCameraConstants {
		Matrix4x4 view;
		Matrix4x4 inv_projection;
	};
};

}