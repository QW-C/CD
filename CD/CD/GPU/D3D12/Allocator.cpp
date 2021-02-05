#include <CD/GPU/D3D12/Allocator.hpp>

namespace CD::GPU::D3D12 {

constexpr std::uint64_t default_heap_size = 1ui64 << 28;

struct Heap {
	ID3D12Heap* heap;
	std::uint64_t current_offset;
	std::uint64_t size;
	std::uint32_t ref_count;
};

struct HeapMemory {
	HeapPool* allocator;
	Heap* parent;
	std::uint64_t offset;
	std::uint64_t size;
	std::uint32_t ref_count;
};

HeapPool::HeapPool() :
	adapter(),
	heap_desc() {
}

HeapPool::~HeapPool() {
	for(auto&& heap : heaps) {
		heap->heap->Release();
	}
}

void HeapPool::init(const Adapter& adapter, const D3D12_HEAP_DESC& heap_desc) {
	this->adapter = &adapter;
	this->heap_desc = heap_desc;
}

HeapMemory* HeapPool::allocate(const D3D12_RESOURCE_ALLOCATION_INFO& info) {
	CD_ASSERT(info.SizeInBytes <= heap_desc.SizeInBytes);

	for(auto& allocation : memory_pool) {
		Heap* heap = allocation->parent;
		if(allocation->ref_count == 0 && info.SizeInBytes <= allocation->size && allocation->offset % info.Alignment == 0) {
			++allocation->ref_count;
			return allocation.get();
		}
	}

	for(auto& heap : heaps) {
		if(std::size_t aligned_offset = align(heap->current_offset, info.Alignment); aligned_offset + info.SizeInBytes <= heap_desc.SizeInBytes) {
			heap->current_offset = aligned_offset + info.SizeInBytes;
			return create_block(*heap, aligned_offset, info.SizeInBytes);
		}
	}

	Heap* heap = create_heap();
	heap->current_offset = info.SizeInBytes;
	return create_block(*heap, 0, info.SizeInBytes);
}

void HeapPool::deallocate(HeapMemory* allocation) {
	CD_ASSERT(allocation->ref_count);
	--allocation->ref_count;
	if(allocation->ref_count == 0) {
		CD_ASSERT(allocation->parent->ref_count);
		--allocation->parent->ref_count;
	}

	if(Heap* heap = allocation->parent; allocation->offset == heap->current_offset) {
		heap->current_offset -= allocation->size;
	}
}

HeapMemory* HeapPool::create_block(Heap& heap, std::uint64_t offset, std::uint64_t size) {
	++heap.ref_count;

	auto& allocation = memory_pool.emplace_back(std::make_unique<HeapMemory>());
	allocation->allocator = this;
	allocation->parent = &heap;
	allocation->offset = offset;
	allocation->ref_count = 1;
	allocation->size = size;

	return allocation.get();
}

Heap* HeapPool::create_heap() {
	auto& heap = heaps.emplace_back(std::make_unique<Heap>());

	HR_ASSERT(adapter->device->CreateHeap(&heap_desc, IID_PPV_ARGS(&heap->heap)));
	heap->size = heap_desc.SizeInBytes;

	return heap.get();
}

Allocator::Allocator(const Adapter& adapter) :
	adapter(adapter) {
	for(std::size_t type = 0; type < HeapPoolType_Count; ++type) {
		D3D12_HEAP_DESC heap_desc {};
		heap_desc.SizeInBytes = default_heap_size;
		heap_desc.Properties.CreationNodeMask = 1 << adapter.node_index;
		heap_desc.Properties.VisibleNodeMask = 1 << adapter.node_index;
		//heap_desc.Flags = D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;

		switch(type) {
		case HeapPoolType_UploadHeap:
			heap_desc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
			heap_desc.Flags |= D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			break;
		case HeapPoolType_ReadbackHeap:
			heap_desc.Properties.Type = D3D12_HEAP_TYPE_READBACK;
			heap_desc.Flags |= D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			break;
		case HeapPoolType_Buffer:
			heap_desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
			heap_desc.Flags |= D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			break;
		case HeapPoolType_RenderTarget:
			heap_desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
			heap_desc.Flags |= D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
			break;
		case HeapPoolType_Texture:
			heap_desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
			heap_desc.Flags |= D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
			break;
		case HeapPoolType_MSAA:
			heap_desc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
			heap_desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
			heap_desc.Flags |= D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
			break;
		}

		heap_pools[type].init(adapter, heap_desc);
	}
}

Buffer Allocator::create_buffer(const D3D12_RESOURCE_DESC& desc, D3D12_HEAP_TYPE type, D3D12_RESOURCE_STATES state) {
	Buffer buffer {};
	buffer.parent = create_resource(buffer.resource, desc, type, state);
	buffer.va = buffer.resource->GetGPUVirtualAddress();

	return buffer;
}

Texture Allocator::create_texture(const D3D12_RESOURCE_DESC& desc, D3D12_HEAP_TYPE type, D3D12_RESOURCE_STATES state) {
	Texture texture {};
	texture.parent = create_resource(texture.resource, desc, type, state);
	texture.rtv = {~0ui64};
	texture.dsv_write = {~0ui64};
	texture.dsv_read = {~0ui64};
	
	return texture;
}

void Allocator::destroy_buffer(Buffer& buffer) {
	destroy_resource(buffer.parent);
}

void Allocator::destroy_texture(Texture& texture) {
	destroy_resource(texture.parent);
}

HeapMemory* Allocator::create_resource(ID3D12Resource*& resource, const D3D12_RESOURCE_DESC& desc, D3D12_HEAP_TYPE type, D3D12_RESOURCE_STATES state) {
	HeapPoolType pool;
	if(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
		switch(type) {
		case D3D12_HEAP_TYPE_UPLOAD:
			pool = HeapPoolType_UploadHeap;
			break;
		case D3D12_HEAP_TYPE_READBACK:
			pool = HeapPoolType_ReadbackHeap;
			break;
		default:
			pool = HeapPoolType_Buffer;
			break;
		}
	}
	else {
		CD_ASSERT(type == D3D12_HEAP_TYPE_DEFAULT);
		if(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL || desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) {
			pool = HeapPoolType_RenderTarget;
		}
		else {
			pool = HeapPoolType_Texture;
		}
	}

	D3D12_RESOURCE_ALLOCATION_INFO info = adapter.device->GetResourceAllocationInfo(1 << adapter.node_index, 1, &desc);
	HeapMemory* memory = heap_pools[pool].allocate(info);

	HR_ASSERT(adapter.device->CreatePlacedResource(memory->parent->heap, memory->offset, &desc, state, nullptr, IID_PPV_ARGS(&resource)));

	return memory;
}

void Allocator::destroy_resource(HeapMemory* memory) {
	memory->allocator->deallocate(memory);
}

}