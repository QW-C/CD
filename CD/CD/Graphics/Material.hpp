#pragma once

#include <CD/Graphics/Common.hpp>
#include <CD/Common/Debug.hpp>
#include <vector>

namespace CD {

class MaterialSystem {
public:
	MaterialSystem(GPU::Device&);
	~MaterialSystem();

	std::uint16_t create_material(const GPU::TextureView* views, std::uint32_t num_descriptors);

	GPU::PipelineHandle get_resource_list() const;

	static constexpr std::uint32_t texture_list_size = 1 << 16;
private:
	static constexpr std::uint32_t max_textures = 6;

	GPU::Device& device;

	GPU::PipelineHandle texture_list;
	std::uint32_t current_offset;
};

struct MaterialConstants {
	std::uint32_t index;
};

enum MaterialTextureIndex : std::uint8_t {
	MaterialTextureIndex_Albedo,
	MaterialTextureIndex_Normal,
	MaterialTextureIndex_Metallicity,
	MaterialTextureIndex_Roughness,
	MaterialTextureIndex_Count
};

struct MaterialInstanceDesc {
	GPU::TextureView textures[MaterialTextureIndex_Count];
};

class MaterialInstance {
public:
	MaterialInstance(MaterialSystem&, const MaterialInstanceDesc&);

	const MaterialConstants& get_constants() const;
private:
	std::uint16_t instance_id;
	MaterialSystem& allocator;

	MaterialConstants constants;

	/*std::uint32_t get_hash() const {
		std::uint16_t pipeline_hash = static_cast<std::uint16_t>(graphics_pipeline.handle);
		return (static_cast<uint32_t>(instance_id) << 16) | pipeline_hash;
	}*/
};

}