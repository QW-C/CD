#pragma once

#include <CD/Graphics/Material.hpp>
#include <CD/Graphics/Frame.hpp>

namespace CD {

enum MeshVertexElement : std::uint8_t {
	MeshVertexElement_Position,
	MeshVertexElement_Normal,
	MeshVertexElement_Tangent,
	MeshVertexElement_TexCoords,
	MeshVertexElement_Count
};

struct IndexedInputBuffer {
	GPU::BufferHandle input_buffer;
	std::uint32_t num_vertices;
	GPU::BufferHandle index_buffer;
	std::uint32_t num_indices;
	std::uint32_t index_buffer_size;
	std::uint32_t vertex_buffer_offsets[MeshVertexElement_Count];
	std::uint32_t vertex_buffer_strides[MeshVertexElement_Count];
	std::uint32_t vertex_buffer_sizes[MeshVertexElement_Count];
};

class Mesh {
public:
	Mesh(GPU::Device&, std::unique_ptr<IndexedInputBuffer>&&, const AABB&);
	~Mesh();

	const IndexedInputBuffer& get_input_buffer() const;
	const AABB& get_aabb() const;
private:
	GPU::Device& device;

	AABB aabb;
	std::unique_ptr<IndexedInputBuffer> input_buffer;
};

class Model {
public:
	Model();
	void add_mesh(const Mesh*, const MaterialInstance*);

	const std::vector<const Mesh*>& get_meshes() const;
	const std::vector<const MaterialInstance*>& get_materials() const;
private:
	std::vector<const Mesh*> meshes;
	std::vector<const MaterialInstance*> materials;
};

}