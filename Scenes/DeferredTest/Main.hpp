#pragma once

#include <CD/Loader/Main.hpp>
#include <CD/Common/Clock.hpp>
#include <CD/Common/Window.hpp>
#include <CD/Loader/ResourceLoader.hpp>
#include <CD/GPU/D3D12/Factory.hpp>
#include <CD/Graphics/GraphicsManager.hpp>

using namespace CD;

class CameraController {
public:
	CameraController(Camera&);

	void update();
	void move(const Vector3& dv);
	void rotate(float x, float y);
private:
	Camera& camera;

	Transform transform;

	Vector3 dp;
	float dx;
	float dy;
};

struct SceneItem {
	const Model* model;
	Transform transform;
};

class DeferredTest : public Main {
public:
	DeferredTest();
	~DeferredTest();

	void run() final;
private:
	Clock clock;

	std::unique_ptr<Window> window;

	GPU::D3D12::Factory factory;
	GPU::Device* device;

	std::unique_ptr<GraphicsManager> graphics;

	std::vector<SceneItem> items;
	std::unique_ptr<CameraController> camera;

	std::unique_ptr<ResourceLoader> resource_loader;
};