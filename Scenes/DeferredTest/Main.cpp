#include <DeferredTest/Main.hpp>

CameraController::CameraController(Camera& camera) :
	camera(camera),
	transform(transform_identity()),
	dp(),
	dx(),
	dy() {
}

void CameraController::update() {
	using namespace DirectX;

	XMVECTOR q = XMLoadFloat4(&transform.orientation);
	q = XMQuaternionRotationRollPitchYaw(dy, dx, 0.f);
	XMStoreFloat4(&transform.orientation, q);

	/*
	XMVECTOR x {1.f};
	XMVECTOR y {0.f, 1.f};
	XMVECTOR qx = XMQuaternionRotationAxis(y, dx);
	XMVECTOR qy = XMQuaternionRotationAxis(x, dy);
	q = XMQuaternionMultiply(q, qx);
	q = XMQuaternionMultiply(q, qy);
	*/

	XMVECTOR v = XMLoadFloat3(&transform.position);
	XMVECTOR d = XMLoadFloat3(&dp);
	XMVECTOR dir = XMVector3Rotate(d, q);
	v += dir;
	XMStoreFloat3(&transform.position, v);

	camera.update_transform(transform);

	dp = {};
}

void CameraController::move(const Vector3& dv) {
	dp = {dp.x + dv.x, dp.y + dv.y, dp.z + dv.z};
}

void CameraController::rotate(float x, float y) {
	using namespace DirectX;
	dx += x;
	dy += y;
	dx = XMScalarModAngle(dx);
	dy = XMScalarModAngle(dy);
}

DeferredTest::DeferredTest() {
	unsigned width = 1280;
	unsigned height = 720;
	DisplayMode display {width, height};

	window = std::make_unique<Window>(display, L"Deferred");
	window->set_cursor(true);

	GPU::SwapChainDesc swapchain {};
	swapchain.handle = window->get_handle();
	swapchain.width = width;
	swapchain.height = height;

	GPU::CreateDeviceDesc device_desc {};
	device_desc.swapchain = &swapchain;
	device_desc.allow_uma = true;

	device = nullptr;
	for(std::uint32_t i = 0; !device && i < factory.get_device_count(); device = factory.create_device(device_desc, i), ++i);

	if(!device) {
		CD_FAIL("no compatible device found");
	}

	graphics = std::make_unique<GraphicsManager>(*device, float(width), float(height));

	resource_loader = std::make_unique<ResourceLoader>(graphics->get_frame());

	window->set_resize_callback([this](float width, float height) {
		graphics->get_frame().resize_buffers(width, height);
		graphics->get_render_pipeline().create_resources();
	});

	Scene& scene = graphics->get_scene();

	Camera& scene_camera = scene.get_camera();
	scene_camera.set_parameters(.25f * std::acosf(-1.f), float(width) / height, 0.1f, 1000.f);

	camera = std::make_unique<CameraController>(scene_camera);
	camera->move({0.f, 0.f, -5.f});

	Light& light = scene.add_light();
	light.position = Vector4(0.f, 0.f, -5.f, 1.f);
	light.color = Vector4(1.f, 1.f, 1.f, 0.f);
	light.intensity = 10.f;

	const wchar_t* material_paths[] {
		L"Assets/albedo.dds",
		L"Assets/normal.dds",
		L"Assets/metallicity.dds",
		L"Assets/roughness.dds"
	};
	MaterialInstanceDesc material_desc {};
	for(std::size_t i = 0; i < MaterialTextureIndex_Count; ++i) {
		const Texture* texture = resource_loader->load_texture2d(material_paths[i], GPU::BufferFormat::R8G8B8A8_UNORM, GPU::TextureDimension::Texture2D);
		material_desc.textures[i] = GPU::texture_view_defaults(texture->handle, texture->desc);
	}
	MaterialInstance* material = resource_loader->create_material(graphics->get_material_system(), material_desc);

	const Model* model = resource_loader->load_model("Assets/model.obj", *material);

	for(float x = -25.f; x < 25.f; x += 5.f) {
		for(float y = -25.f; y < 25.f; y += 5.f) {
			Transform transform = transform_identity();
			transform.position = {x, y, 0.f};
			items.push_back({model, transform});
		}
	}

	const Texture* sky_texture = resource_loader->load_texture2d(L"Assets/cubemap.dds", GPU::BufferFormat::R8G8B8A8_UNORM, GPU::TextureDimension::Texture2D);
	Sky& sky = graphics->get_sky();
	sky.set_texture(sky_texture);
}

DeferredTest::~DeferredTest() = default;

void DeferredTest::run() {
	CursorPosition pos = window->cursor_position();
	float pf = 100.f;
	float rf = 15.f;
	
	while(!window->closed()) {
		clock.reset();
		const float dt = static_cast<float>(clock.get_elapsed_time_ms());

		window->poll_messages();

		if(window->key_state(Key::Q) == KeyState::Pressed || window->key_state(Key::ESCAPE) == KeyState::Pressed) {
			window->close();
		}

		Vector3 dp {};
		Vector2 dr {};

		if(window->key_state(Key::A) == KeyState::Pressed) {
			dp.x -= pf * dt;
		}
		if(window->key_state(Key::D) == KeyState::Pressed) {
			dp.x += pf * dt;
		}
		if(window->key_state(Key::S) == KeyState::Pressed) {
			dp.z -= pf * dt;
		}
		if(window->key_state(Key::W) == KeyState::Pressed) {
			dp.z += pf * dt;
		}

		const CursorPosition& p = window->cursor_position();
		if(window->button_state(Button::MB1) == KeyState::Pressed || window->button_state(Button::MB2) == KeyState::Pressed) {
			dr.x = (p.x - pos.x) * dt * rf;
			dr.y = (p.y - pos.y) * dt * rf;
		}

		pos = p;

		camera->move(dp);
		camera->rotate(dr.x, dr.y);
		camera->update();

		Renderer& renderer = graphics->get_renderer();
		renderer.clear_render_state();

		for(auto& item : items) {
			renderer.add_model(item.model, item.transform);
		}

		graphics->render();
	}

	graphics->get_frame().wait();
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
	DeferredTest app;
	app.run();
	return 0;
}