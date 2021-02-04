#include <CD/GPU/D3D12/Common.hpp>

namespace CD::GPU::D3D12 {

ShaderDescriptorHeap::ShaderDescriptorHeap(const Adapter& adapter) :
	offset(),
	descriptor_count(d3d12_shader_descriptor_heap_size),
	increment(adapter.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)) {
	D3D12_DESCRIPTOR_HEAP_DESC shader_heap_desc {
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		d3d12_shader_descriptor_heap_size,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		1u << adapter.node_index
	};
	HR_ASSERT(adapter.device->CreateDescriptorHeap(&shader_heap_desc, IID_PPV_ARGS(&descriptor_heap)));
	start = descriptor_heap->GetGPUDescriptorHandleForHeapStart();
	cpu_start = descriptor_heap->GetCPUDescriptorHandleForHeapStart();
}

ShaderDescriptorHeap::~ShaderDescriptorHeap() {
	descriptor_heap->Release();
}

DescriptorTable ShaderDescriptorHeap::create_descriptor_table(std::uint32_t num_descriptors) {
	for(auto it = release_list.begin(); it != release_list.end(); ++it) {
		if(it->num_descriptors >= num_descriptors) {
			DescriptorTable table = *it;
			table.num_descriptors = num_descriptors;
			release_list.erase(it);
			return table;
		}
	}

	CD_ASSERT(offset + num_descriptors <= descriptor_count);

	std::uint64_t base = offset * increment;
	offset += num_descriptors;
	return {cpu_start.ptr + base, start.ptr + base, num_descriptors};
}

void ShaderDescriptorHeap::destroy_descriptor_table(DescriptorTable& table) {
	release_list.push_back(table);
}

ID3D12DescriptorHeap* ShaderDescriptorHeap::get_descriptor_heap() const {
	return descriptor_heap;
}

std::uint32_t ShaderDescriptorHeap::get_increment() const {
	return increment;
}

DescriptorPool::DescriptorPool(const Adapter& adapter, std::uint32_t num_descriptors, D3D12_DESCRIPTOR_HEAP_TYPE type) :
	adapter(adapter),
	desc(),
	start(),
	num_descriptors(),
	increment(adapter.device->GetDescriptorHandleIncrementSize(type)) {
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.Type = type;
	desc.NumDescriptors = num_descriptors;
	desc.NodeMask = 1 << adapter.node_index;
}

DescriptorPool::~DescriptorPool() {
	for(ID3D12DescriptorHeap* heap : descriptor_heaps) {
		heap->Release();
	}
}

CPUHandle DescriptorPool::add_descriptor() {
	if(release_list.size()) {
		CPUHandle handle = release_list.back();
		release_list.pop_back();
		return handle;
	}

	if(CPUHandle descriptor = {start.ptr + increment * (desc.NumDescriptors - num_descriptors)}; !descriptor_heaps.empty() && num_descriptors) {
		--num_descriptors;
		return descriptor;
	}

	ID3D12DescriptorHeap* heap = nullptr;
	HR_ASSERT(adapter.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));

	num_descriptors = desc.NumDescriptors - 1;

	start = heap->GetCPUDescriptorHandleForHeapStart();
	descriptor_heaps.emplace_back(heap);

	return start;
}

void DescriptorPool::remove_descriptor(CPUHandle descriptor) {
	release_list.push_back(descriptor);
}

constexpr std::size_t default_pool_size = 1 << 16;

DeviceResources::DeviceResources() :
	buffer_pool(default_pool_size),
	texture_pool(default_pool_size),
	pipeline_state_pool(default_pool_size),
	descriptor_table_pool(default_pool_size),
	render_pass_pool(default_pool_size) {
}

}