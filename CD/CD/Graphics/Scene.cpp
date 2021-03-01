#include <CD/Graphics/Scene.hpp>

namespace CD {

Camera::Camera() :
	transform(),
	world_to_view(),
	view_projection(),
	perspective(),
	inv_projection(),
	inv_view_projection() {
}

const Matrix4x4& Camera::get_projection() const {
	return perspective;
}

const Matrix4x4& Camera::get_transform() const {
	return transform;
}

const Matrix4x4& Camera::get_view() const {
	return world_to_view;
}

const Matrix4x4& Camera::get_view_projection() const {
	return view_projection;
}

const Matrix4x4& Camera::get_inv_projection() const {
	return inv_projection;
}

const Matrix4x4& Camera::get_inv_view_projection() const {
	return inv_view_projection;
}

void Camera::update_transform(const Transform& t) {
	using namespace DirectX;

	transform = matrix_transform(t);

	XMMATRIX tm = XMLoadFloat4x4(&transform);
	XMMATRIX inv = XMMatrixInverse(nullptr, tm);
	XMStoreFloat4x4(&world_to_view, inv);

	update();
}

void Camera::set_parameters(float fov, float ar, float nz, float fz) {
	perspective = matrix_perspective(fov, ar, nz, fz);

	update();
}

void Camera::update() {
	using namespace DirectX;

	XMMATRIX t = XMLoadFloat4x4(&transform);
	XMMATRIX v = XMLoadFloat4x4(&world_to_view);
	XMMATRIX p = XMLoadFloat4x4(&perspective);

	XMMATRIX vp = v * p;
	XMStoreFloat4x4(&view_projection, vp);

	XMMATRIX inv_p = XMMatrixInverse(nullptr, p);
	XMStoreFloat4x4(&inv_projection, inv_p);

	XMMATRIX inv_vp = inv_p * t;
	XMStoreFloat4x4(&inv_view_projection, inv_vp);
}

Scene::Scene(Frame& frame) :
	frame(frame),
	camera(),
	light_buffer(),
	parameters() {
}

void Scene::update_data() {
	GPUBufferAllocator& allocator = frame.get_buffer_allocator();

	Light buffer[max_lights] {};

	for(std::size_t i = 0; i < lights.size(); ++i) {
		buffer[i] = *lights[i];
	}

	light_buffer = allocator.create_buffer(static_cast<std::uint32_t>(sizeof(Light) * lights.size()), buffer, false);

	const GPU::Viewport& vp = frame.get_viewport();

	LightingParametersBuffer parameters_buffer {
		camera.get_inv_view_projection(),
		camera.get_transform(),
		Vector2(1.f / vp.width, 1.f / vp.height),
		static_cast<std::uint32_t>(lights.size())
	};
	parameters = allocator.create_buffer(sizeof(LightingParametersBuffer), &parameters_buffer, false);
}

Light& Scene::add_light() {
	return *lights.emplace_back(std::make_unique<Light>());
}

Camera& Scene::get_camera() {
	return camera;
}

const BufferAllocation& Scene::get_light_buffer() const {
	return light_buffer;
}

const BufferAllocation& Scene::get_lighting_parameters() const {
	return parameters;
}

}