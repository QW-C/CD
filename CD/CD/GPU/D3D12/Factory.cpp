#include <CD/GPU/D3D12/Factory.hpp>
#include <DXProgrammableCapture.h>

namespace CD::GPU::D3D12 {

constexpr D3D_FEATURE_LEVEL d3d_feature_levels[] {
	D3D_FEATURE_LEVEL_12_1,
	D3D_FEATURE_LEVEL_12_0,
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_11_0
};

Factory::Factory() :
	factory(nullptr),
	debug_layer_enabled(false) {

	IDXGraphicsAnalysis* ga = nullptr;
	if(ID3D12Debug1* debug_controller = nullptr; DXGIGetDebugInterface1(0, IID_PPV_ARGS(&ga)) == E_NOINTERFACE) {
		if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
			debug_controller->EnableDebugLayer();
			debug_controller->Release();

			debug_layer_enabled = true;
		}
	}

	HR_ASSERT(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));

	IDXGIAdapter1* adapter = nullptr;
	for(std::uint32_t adapter_index = 0; factory->EnumAdapters1(adapter_index, &adapter) != DXGI_ERROR_NOT_FOUND; ++adapter_index) {
		DXGI_ADAPTER_DESC1 desc {};
		adapter->GetDesc1(&desc);

		if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}

		D3D12_FEATURE_DATA_FEATURE_LEVELS feature_level {static_cast<UINT>(std::size(d3d_feature_levels)), d3d_feature_levels, D3D_FEATURE_LEVEL_11_0};
		ID3D12Device* device = nullptr;
		if(SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) {
			HR_ASSERT(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feature_level, sizeof(feature_level)));

			D3D12_FEATURE_DATA_ARCHITECTURE1 architecture1 {};
			HR_ASSERT(device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE1, &architecture1, sizeof(architecture1)));

			DeviceFeatureInfo info {};
			info.uma = architecture1.UMA;

			IDXGIAdapter3* dxgi_adapter = nullptr;
			HR_ASSERT(adapter->QueryInterface<IDXGIAdapter3>(&dxgi_adapter));
			adapter->Release();

			UINT node_count = device->GetNodeCount();

			for(std::uint32_t node_index = 0; node_index < node_count; ++node_index) {
				auto& adapter_ptr = adapters.emplace_back(std::make_unique<Adapter>());
				adapter_ptr->adapter = dxgi_adapter;
				adapter_ptr->node_index = node_index;
				adapter_ptr->feature_info = info;
				adapter_ptr->feature_level = feature_level.MaxSupportedFeatureLevel;
			}

			device->Release();
		}
	}
}

Factory::~Factory() {
	factory->Release();
}

std::size_t Factory::get_device_count() const {
	return adapters.size();
}

Device* Factory::create_device(const CreateDeviceDesc& desc, std::size_t index) {
	if(index >= adapters.size()) {
		return nullptr;
	}

	Adapter& adapter = *adapters[index];
	if(adapter.feature_info.uma && !desc.allow_uma) {
		return nullptr;
	}
	
	HR_ASSERT(D3D12CreateDevice(adapter.adapter, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&adapter.device)));

	if(debug_layer_enabled) {
		ID3D12InfoQueue* info_queue = nullptr;
		HR_ASSERT(adapter.device->QueryInterface(&info_queue));

		D3D12_MESSAGE_ID silenced_list[] {
			D3D12_MESSAGE_ID_CREATEINPUTLAYOUT_EMPTY_LAYOUT
		};

		D3D12_INFO_QUEUE_FILTER_DESC deny_list {
			0,
			nullptr,
			0,
			nullptr,
			static_cast<UINT>(std::size(silenced_list)),
			silenced_list
		};

		D3D12_INFO_QUEUE_FILTER deny_filter {
			{},
			deny_list
		};

		info_queue->AddStorageFilterEntries(&deny_filter);

		info_queue->Release();
	}

	return devices.emplace_back(std::make_unique<Device>(adapter, compiler, desc.swapchain, factory)).get();
}

}