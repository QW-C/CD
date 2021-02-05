#pragma once

#include <CD/GPU/D3D12/Common.hpp>
#include <vector>
#include <memory>

namespace CD::GPU::D3D12 {

struct Heap;
struct HeapMemory;

class HeapPool {
public:
	HeapPool();
	~HeapPool();

	void init(const Adapter&, const D3D12_HEAP_DESC&);
	HeapMemory* allocate(const D3D12_RESOURCE_ALLOCATION_INFO&);
	void deallocate(HeapMemory*);
private:
	const Adapter* adapter;
	D3D12_HEAP_DESC heap_desc;
	std::vector<std::unique_ptr<HeapMemory>> memory_pool;
	std::vector<std::unique_ptr<Heap>> heaps;

	HeapMemory* create_block(Heap&, std::uint64_t offset, std::uint64_t size);
	Heap* create_heap();
};

class Allocator {
public:
	Allocator(const Adapter&);

	Buffer create_buffer(const D3D12_RESOURCE_DESC&, D3D12_HEAP_TYPE, D3D12_RESOURCE_STATES);
	Texture create_texture(const D3D12_RESOURCE_DESC&, D3D12_HEAP_TYPE, D3D12_RESOURCE_STATES);

	void destroy_buffer(Buffer&);
	void destroy_texture(Texture&);
private:
	enum HeapPoolType : std::uint8_t {
		HeapPoolType_UploadHeap,
		HeapPoolType_ReadbackHeap,
		HeapPoolType_Buffer,
		HeapPoolType_RenderTarget,
		HeapPoolType_Texture,
		HeapPoolType_MSAA,
		HeapPoolType_Count
	};

	const Adapter& adapter;

	HeapPool heap_pools[HeapPoolType_Count];

	HeapMemory* create_resource(ID3D12Resource*&, const D3D12_RESOURCE_DESC&, D3D12_HEAP_TYPE, D3D12_RESOURCE_STATES);
	void destroy_resource(HeapMemory*);
};

}