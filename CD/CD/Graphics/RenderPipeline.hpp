#pragma once

#include <CD/Graphics/Frame.hpp>
#include <CD/Graphics/Lighting.hpp>
#include <vector>

namespace CD {

class ScopedRenderPass {
public:
	ScopedRenderPass(GPU::CommandBuffer&, const GPU::PipelineHandle& render_pass, const GPU::TextureView* color_buffers, std::uint32_t num_render_targets, const GPU::TextureView& depth_stencil, bool depth_write, const GPU::Viewport&);
	ScopedRenderPass(const ScopedRenderPass&) = delete;
	ScopedRenderPass(ScopedRenderPass&&) = delete;
	ScopedRenderPass& operator=(const ScopedRenderPass&) = delete;
	ScopedRenderPass& operator=(ScopedRenderPass&&) = delete;
	~ScopedRenderPass();
private:
	GPU::CommandBuffer& command_buffer;
};

class Renderer;
class Sky;

class RenderPipeline {
public:
	RenderPipeline(Frame&, Renderer&, Sky&, MaterialSystem&);

	void render(Scene&);

	void create_render_passes();
	void create_resources();
private:
	Frame& frame;
	Renderer& renderer;
	Sky& sky;
	Lighting lighting;
	Tonemapper tonemapper;

	struct DepthPass {
		const RenderPass* render_pass;

		FrameResourceIndex depth_buffer;

		GPU::TextureView view_desc;
	};

	struct GBuffer {
		const RenderPass* render_pass;

		FrameResourceIndex normals;
		FrameResourceIndex uv;
		FrameResourceIndex duv;
		FrameResourceIndex material_indices;

		GPU::TextureView view_desc[4];
	};

	struct SkyboxPass {
		const RenderPass* render_pass;
	};

	DepthPass depth_pass;
	GBuffer gbuffer;
	SkyboxPass sky_pass;

	FrameResourceIndex lighting_out;
	FrameResourceIndex final_image;

	GPU::PipelineHandle geometry_view;

	void execute_depth();
	void execute_geometry();
	void execute_lighting(Scene&);
	void execute_tonemapping();
	void execute_sky(Scene&);
	void execute_present();
};

}