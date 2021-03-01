#include <CD/Graphics/Material.hpp>

namespace CD {

MaterialSystem::MaterialSystem(GPU::Device& device) :
	device(device),
	texture_list(device.create_pipeline_input_list(texture_list_size)),
	current_offset(max_textures) {

	GPU::TextureDesc null = GPU::texture_desc_defaults(1, 1, GPU::BufferFormat::R8G8B8A8_UINT);
	GPU::TextureView null_view = GPU::texture_view_defaults(GPU::TextureHandle::Null, null);
	GPU::TextureView views[max_textures] {};
	std::fill_n(views, max_textures, null_view);
	device.update_pipeline_input_list(texture_list, GPU::DescriptorType::SRV, views, max_textures, 0);
}

MaterialSystem::~MaterialSystem() {
	device.destroy_pipeline_resource(texture_list);
}

std::uint16_t MaterialSystem::create_material(const GPU::TextureView* views, std::uint32_t num_descriptors) {
	CD_ASSERT(current_offset + num_descriptors < texture_list_size);
	CD_ASSERT(num_descriptors <= max_textures);

	device.update_pipeline_input_list(texture_list, GPU::DescriptorType::SRV, views, num_descriptors, current_offset);

	std::uint16_t offset = current_offset;
	current_offset += num_descriptors;

	return offset;
}

GPU::PipelineHandle MaterialSystem::get_resource_list() const {
	return texture_list;
}

MaterialInstance::MaterialInstance(MaterialSystem& allocator, const MaterialInstanceDesc& desc) :
	allocator(allocator) {

	instance_id = allocator.create_material(desc.textures, MaterialTextureIndex_Count);
	constants = {instance_id};
}

const MaterialConstants& MaterialInstance::get_constants() const {
	return constants;
}

}