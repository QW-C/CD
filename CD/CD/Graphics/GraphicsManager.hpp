#pragma once

#include <CD/Graphics/RenderPipeline.hpp>
#include <CD/Graphics/Renderer.hpp>
#include <CD/Graphics/Sky.hpp>
#include <CD/Graphics/Frame.hpp>
#include <CD/GPU/Device.hpp>

namespace CD {

class GraphicsManager {
public:
	GraphicsManager(GPU::Device&, float width, float height);

	Frame& get_frame();
	RenderPipeline& get_render_pipeline();
	Renderer& get_renderer();
	Scene& get_scene();
	MaterialSystem& get_material_system();
	Sky& get_sky();

	void render();
	void resize(float width, float height);
private:
	GPU::Device& device;

	Frame frame;

	Scene scene;
	MaterialSystem material_system;
	Renderer renderer;
	Sky sky;
	RenderPipeline render_pipeline;
};

}