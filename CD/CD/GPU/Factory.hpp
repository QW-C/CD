#pragma once

#include <CD/Common/Common.hpp>
#include <CD/GPU/Device.hpp>

namespace CD::GPU {

struct CreateDeviceDesc {
	const SwapChainDesc* swapchain;
	bool allow_uma;
};

class Factory {
public:
	Factory() = default;
	Factory(const Factory&) = delete;
	Factory(Factory&&) = delete;
	Factory& operator=(const Factory&) = delete;
	Factory& operator=(Factory&&) = delete;
	virtual ~Factory() = default;

	virtual Device* create_device(const CreateDeviceDesc&, std::size_t adapter_index) = 0;
	virtual std::size_t get_device_count() const = 0;
};

}