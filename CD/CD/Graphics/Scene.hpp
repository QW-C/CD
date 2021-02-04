#pragma once

#include <CD/Graphics/Frame.hpp>
#include <vector>
#include <memory>

namespace CD {

class Camera {
public:
	Camera();

	const Matrix4x4& get_projection() const;
	const Matrix4x4& get_transform() const;
	const Matrix4x4& get_view_projection() const;
	const Matrix4x4& get_inv_projection() const;
	const Matrix4x4& get_inv_view_projection() const;

	void update_transform(const Transform&);

	void set_parameters(float fov, float ar, float nz, float fz);
private:
	Matrix4x4 transform;
	Matrix4x4 world_to_view;
	Matrix4x4 perspective;
	Matrix4x4 view_projection;
	Matrix4x4 inv_projection;
	Matrix4x4 inv_view_projection;

	void update();
};

struct Light {
	Vector4 position;
	Vector4 color;
	float intensity;
	float unused[3];
};

class Scene {
public:
	Scene(Frame&);

	void update_data();

	Light& add_light();

	Camera& get_camera();

	const BufferAllocation& get_light_buffer() const;
	const BufferAllocation& get_lighting_parameters() const;
private:
	constexpr static std::size_t max_lights = 64;

	Frame& frame;

	Camera camera;

	struct LightingParametersBuffer {
		Matrix4x4 inv_view_projection;
		Matrix4x4 camera_transform;
		Vector2 inv_resolution;
		std::uint32_t count;
	};

	std::vector<std::unique_ptr<Light>> lights;
	BufferAllocation light_buffer;
	BufferAllocation parameters;
};

}