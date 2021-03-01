#include <CD/Graphics/Model.hpp>

namespace CD {

Mesh::Mesh(GPU::Device& device, std::unique_ptr<IndexedInputBuffer>&& buffer, const AABB& aabb) :
	device(device),
	input_buffer(std::move(buffer)),
	aabb(aabb) {
}

Mesh::~Mesh() {
	device.destroy_buffer(input_buffer->input_buffer);
	device.destroy_buffer(input_buffer->index_buffer);
}

const IndexedInputBuffer& Mesh::get_input_buffer() const {
	return *input_buffer;
}

const AABB& Mesh::get_aabb() const {
	return aabb;
}

Model::Model() = default;

void Model::add_mesh(const Mesh* mesh, const MaterialInstance* material) {
	meshes.push_back(mesh);
	materials.push_back(material);
}

const std::vector<const Mesh*>& Model::get_meshes() const {
	return meshes;
}

const std::vector<const MaterialInstance*>& Model::get_materials() const {
	return materials;
}

}