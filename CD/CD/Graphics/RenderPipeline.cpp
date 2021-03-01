#include <CD/Graphics/RenderPipeline.hpp>
#include <CD/Graphics/Renderer.hpp>
#include <CD/Graphics/Sky.hpp>
#include <CD/Graphics/Frame.hpp>

namespace CD {

ScopedRenderPass::ScopedRenderPass(GPU::CommandBuffer& command_buffer, const GPU::PipelineHandle& render_pass, const GPU::TextureView* color_buffers, std::uint32_t num_render_targets, const GPU::TextureView& depth_stencil, bool depth_write, const GPU::Viewport& viewport) :
	command_buffer(command_buffer) {
	GPU::BeginRenderPassDesc begin_render_pass {};

	begin_render_pass.render_pass = render_pass;

	for(std::size_t i = 0; i < num_render_targets; ++i) {
		begin_render_pass.color[i] = color_buffers[i];
	}
	begin_render_pass.render_target_count = num_render_targets;

	begin_render_pass.depth_stencil_target = depth_stencil;
	begin_render_pass.depth_write = depth_write;

	begin_render_pass.viewport = viewport;
	begin_render_pass.primitive_topology = GPU::PrimitiveTopology::TriangleList;

	command_buffer.add_command(begin_render_pass);
}

ScopedRenderPass::~ScopedRenderPass() {
	GPU::EndRenderPassDesc end_render_pass;
	command_buffer.add_command(end_render_pass);
}

RenderPipeline::RenderPipeline(Frame& frame, Renderer& renderer, Sky& sky, MaterialSystem& material_system) :
	frame(frame),
	renderer(renderer),
	sky(sky),
	lighting(frame, material_system),
	tonemapper(frame) {

	geometry_view = frame.get_device().create_pipeline_input_list(5);

	create_render_passes();
	create_resources();
}

void RenderPipeline::render(Scene& scene) {
	execute_depth();
	execute_geometry();
	execute_lighting(scene);
	execute_tonemapping();
	execute_sky(scene);
	execute_present();
}

void RenderPipeline::execute_depth() {
	GPU::CommandBuffer& command_buffer = frame.get_command_buffer();

	auto& depth_texture = frame.get_texture(depth_pass.depth_buffer);
	GPU::TextureView depth_view = GPU::texture_view_defaults(depth_texture.texture.handle, depth_texture.texture.desc);

	frame.bind_resources(&depth_pass.depth_buffer, 1, GPU::ResourceState::DepthWrite);

	ScopedRenderPass render_pass(command_buffer, depth_pass.render_pass->handle, nullptr, 0, depth_view, true, frame.get_viewport());

	renderer.draw_depth(command_buffer);
}

void RenderPipeline::execute_geometry() {
	GPU::CommandBuffer& command_buffer = frame.get_command_buffer();

	FrameResourceIndex color_out[] {gbuffer.normals, gbuffer.uv, gbuffer.duv, gbuffer.material_indices};

	const auto& depth_texture = frame.get_texture(depth_pass.depth_buffer);
	GPU::TextureView depth_view = GPU::texture_view_defaults(depth_texture.texture.handle, depth_texture.texture.desc);

	frame.bind_resources(color_out, std::size(color_out), GPU::ResourceState::RTV);
	frame.bind_resources(&depth_pass.depth_buffer, 1, GPU::ResourceState::DepthRead);

	ScopedRenderPass render_pass(command_buffer, gbuffer.render_pass->handle, gbuffer.view_desc, static_cast<std::uint32_t>(std::size(gbuffer.view_desc)), depth_view, false, frame.get_viewport());

	renderer.draw_gbuffer(command_buffer);
}

void RenderPipeline::execute_lighting(Scene& scene) {
	GPU::CommandBuffer& command_buffer = frame.get_command_buffer();

	FrameResourceIndex gbuffer_resources[] {gbuffer.normals, gbuffer.uv, gbuffer.duv, gbuffer.material_indices};

	auto& texture = frame.get_texture(lighting_out);

	frame.bind_resources(gbuffer_resources, std::size(gbuffer_resources), GPU::ResourceState::Common);
	frame.bind_resources(&lighting_out, 1, GPU::ResourceState::UAV);

	lighting.apply(frame.get_command_buffer(), scene, geometry_view, texture.views->uav);
}

void RenderPipeline::execute_tonemapping() {
	GPU::CommandBuffer& command_buffer = frame.get_command_buffer();

	auto& lighting_texture = frame.get_texture(lighting_out);
	auto& final_texture = frame.get_texture(final_image);

	frame.bind_resources(&lighting_out, 1, GPU::ResourceState::Common);
	frame.bind_resources(&final_image, 1, GPU::ResourceState::UAV);

	tonemapper.apply(command_buffer, lighting_texture.views->srv, final_texture.views->uav);
}

void RenderPipeline::execute_sky(Scene& scene) {
	GPU::CommandBuffer& command_buffer = frame.get_command_buffer();

	const auto& depth_texture = frame.get_texture(depth_pass.depth_buffer);
	GPU::TextureView depth_view = GPU::texture_view_defaults(depth_texture.texture.handle, depth_texture.texture.desc);

	auto& render_texture = frame.get_texture(final_image);
	GPU::TextureView render_view = GPU::texture_view_defaults(render_texture.texture.handle, render_texture.texture.desc);

	frame.bind_resources(&final_image, 1, GPU::ResourceState::RTV);
	frame.bind_resources(&depth_pass.depth_buffer, 1, GPU::ResourceState::DepthRead);

	ScopedRenderPass render_pass(command_buffer, sky_pass.render_pass->handle, &render_view, 1, depth_view, false, frame.get_viewport());

	sky.render(command_buffer, scene.get_camera());
}

void RenderPipeline::execute_present() {
	auto& final_texture = frame.get_texture(final_image);

	GPU::CommandBuffer& command_buffer = frame.get_command_buffer();

	GPU::CopyToSwapChainDesc copy;
	copy.texture = GPU::texture_view_defaults(final_texture.texture.handle, final_texture.texture.desc);
	copy.texture_state = final_texture.state;

	command_buffer.add_command(copy);
}

void RenderPipeline::create_render_passes() {
	{
		GPU::RenderPassDesc depth_desc {};
		GPU::RenderPassDepthStencilTargetDesc depth_stencil_desc;
		depth_stencil_desc = GPU::render_pass_depth_stencil_defaults(GPU::RenderPassBeginOp::Clear, GPU::RenderPassEndOp::Preserve, 1.f);
		depth_desc.depth_stencil_target = &depth_stencil_desc;

		depth_pass.render_pass = frame.create_render_pass(depth_desc);
	}

	{
		GPU::RenderPassDesc gbuffer_desc {};
		std::uint32_t num_render_targets = static_cast<std::uint32_t>(std::size(gbuffer_render_target_formats));
		gbuffer_desc.num_render_targets = num_render_targets;

		for(std::size_t i = 0; i < num_render_targets; ++i) {
			gbuffer_desc.render_targets[i].begin_op = GPU::RenderPassBeginOp::Clear;
			gbuffer_desc.render_targets[i].end_op = GPU::RenderPassEndOp::Preserve;
			gbuffer_desc.render_targets[i].clear_format = gbuffer_render_target_formats[i];
		}

		GPU::RenderPassDepthStencilTargetDesc depth_target = GPU::render_pass_depth_stencil_defaults(GPU::RenderPassBeginOp::Preserve);
		gbuffer_desc.depth_stencil_target = &depth_target;

		gbuffer.render_pass = frame.create_render_pass(gbuffer_desc);
	}

	{
		GPU::RenderPassDesc sky_pass_desc {};

		GPU::RenderPassRenderTargetDesc rt {
			GPU::RenderPassBeginOp::Preserve,
			GPU::RenderPassEndOp::Preserve,
			GPU::BufferFormat::R8G8B8A8_UNORM
		};

		GPU::RenderPassDepthStencilTargetDesc ds = GPU::render_pass_depth_stencil_defaults(GPU::RenderPassBeginOp::Preserve);

		sky_pass_desc.num_render_targets = 1;
		sky_pass_desc.render_targets[0] = rt;
		sky_pass_desc.depth_stencil_target = &ds;

		sky_pass.render_pass = frame.create_render_pass(sky_pass_desc);
	}
}

void RenderPipeline::create_resources() {
	const GPU::Viewport& viewport = frame.get_viewport();

	GPU::Device& device = frame.get_device();

	std::uint16_t w = static_cast<std::uint16_t>(viewport.width);
	std::uint16_t h = static_cast<std::uint16_t>(viewport.height);

	{
		GPU::TextureDesc depth_desc = GPU::texture_desc_defaults(w, h, GPU::BufferFormat::D32_FLOAT_S8X24_UINT, GPU::TextureDimension::Texture2D, 1, 1, GPU::BindFlags_DepthStencilTarget);
		depth_pass.depth_buffer = frame.add_texture(depth_desc, false);

		depth_pass.view_desc = GPU::texture_view_defaults(frame.get_texture(depth_pass.depth_buffer).texture.handle, depth_desc);
		depth_pass.view_desc.format = GPU::BufferFormat::R32_FLOAT_X8X24_Typeless;

		GPU::TextureDesc gbuffer_desc[std::size(gbuffer_render_target_formats)] {};
		for(std::size_t i = 0; i < std::size(gbuffer_render_target_formats); ++i) {
			gbuffer_desc[i] = GPU::texture_desc_defaults(w, h, gbuffer_render_target_formats[i], GPU::TextureDimension::Texture2D, 1, 1, GPU::BindFlags_RenderTarget);

		}

		gbuffer.normals = frame.add_texture(gbuffer_desc[0], false);
		gbuffer.uv = frame.add_texture(gbuffer_desc[1], false);
		gbuffer.duv = frame.add_texture(gbuffer_desc[2], false);
		gbuffer.material_indices = frame.add_texture(gbuffer_desc[3], false);

		gbuffer.view_desc[0] = GPU::texture_view_defaults(frame.get_texture(gbuffer.normals).texture.handle, gbuffer_desc[0]);
		gbuffer.view_desc[1] = GPU::texture_view_defaults(frame.get_texture(gbuffer.uv).texture.handle, gbuffer_desc[1]);
		gbuffer.view_desc[2] = GPU::texture_view_defaults(frame.get_texture(gbuffer.duv).texture.handle, gbuffer_desc[2]);
		gbuffer.view_desc[3] = GPU::texture_view_defaults(frame.get_texture(gbuffer.material_indices).texture.handle, gbuffer_desc[3]);

		device.update_pipeline_input_list(geometry_view, GPU::DescriptorType::SRV, gbuffer.view_desc, std::size(gbuffer.view_desc), 0);
		device.update_pipeline_input_list(geometry_view, GPU::DescriptorType::SRV, &depth_pass.view_desc, 1, std::size(gbuffer.view_desc));
	}

	{
		GPU::TextureDesc lighting_output = GPU::texture_desc_defaults(w, h, GPU::BufferFormat::R16G16B16A16_FLOAT, GPU::TextureDimension::Texture2D, 1, 1, GPU::BindFlags_RW);
		lighting_out = frame.add_texture(lighting_output);
	}

	{
		GPU::TextureDesc final_texture_desc = GPU::texture_desc_defaults(w, h, GPU::BufferFormat::R8G8B8A8_UNORM, GPU::TextureDimension::Texture2D, 1, 1, static_cast<GPU::BindFlags>(GPU::BindFlags_RW | GPU::BindFlags_RenderTarget));
		final_image = frame.add_texture(final_texture_desc);
	}
}

}