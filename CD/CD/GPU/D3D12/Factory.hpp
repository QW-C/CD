#pragma once

#include <CD/GPU/Factory.hpp>
#include <CD/GPU/D3D12/Device.hpp>
#include <vector>

namespace CD::GPU::D3D12 {

class Factory : public GPU::Factory {
public:
	Factory();
	~Factory();

	Device* create_device(const CreateDeviceDesc&, std::size_t index) final;
	std::size_t get_device_count() const final;
private:
	IDXGIFactory7* factory;
	bool debug_layer_enabled;

	std::vector<std::unique_ptr<Adapter>> adapters;
	std::vector<std::unique_ptr<Device>> devices;
};

}