#include <CD/Graphics/GraphicsManager.hpp>

namespace CD {

GraphicsManager::GraphicsManager(GPU::Device& device, float width, float height) :
	device(device),
	frame(device, width, height),
	scene(frame),
	material_system(device),
	renderer(frame),
	sky(frame),
	render_pipeline(frame, renderer, sky, material_system) {
}

RenderPipeline& GraphicsManager::get_render_pipeline() {
	return render_pipeline;
}

Frame& GraphicsManager::get_frame() {
	return frame;
}

Scene& GraphicsManager::get_scene() {
	return scene;
}

MaterialSystem& GraphicsManager::get_material_system() {
	return material_system;
}

Renderer& GraphicsManager::get_renderer() {
	return renderer;
}

Sky& GraphicsManager::get_sky() {
	return sky;
}

void GraphicsManager::render() {
	frame.begin();

	scene.update_data();
	renderer.build(scene.get_camera());
	render_pipeline.render(scene);

	frame.present();
}

void GraphicsManager::resize(float width, float height) {
	frame.resize_buffers(width, height);
	render_pipeline.create_resources();
}

}