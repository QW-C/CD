#pragma once

#include <CD/GPU/Device.hpp>
#include <CD/GPU/Utils.hpp>
#include <CD/Common/Transform.hpp>

namespace CD {

constexpr void set_default_samplers(GPU::PipelineInputLayout& layout) {
	layout.samplers[0] = GPU::anisotropic_sampler_defaults(8);
	layout.sampler_binding_slots[0] = 0;
	layout.sampler_binding_spaces[0] = 0;

	layout.samplers[1] = GPU::anisotropic_sampler_defaults(16);
	layout.sampler_binding_slots[1] = 1;
	layout.sampler_binding_spaces[1] = 0;

	layout.samplers[2] = GPU::sampler_defaults(GPU::Filter::Point, GPU::AddressMode::Clamp);
	layout.sampler_binding_slots[2] = 2;
	layout.sampler_binding_spaces[2] = 0;

	layout.samplers[3] = GPU::sampler_defaults(GPU::Filter::Point, GPU::AddressMode::Wrap);
	layout.sampler_binding_slots[3] = 3;
	layout.sampler_binding_spaces[3] = 0;

	layout.samplers[4] = GPU::sampler_defaults(GPU::Filter::Linear, GPU::AddressMode::Clamp);
	layout.sampler_binding_slots[4] = 4;
	layout.sampler_binding_spaces[4] = 0;

	layout.samplers[5] = GPU::sampler_defaults(GPU::Filter::Linear, GPU::AddressMode::Wrap);
	layout.sampler_binding_slots[5] = 5;
	layout.sampler_binding_spaces[5] = 0;

	layout.num_samplers = 6;
}

struct Texture {
	GPU::TextureHandle handle = GPU::TextureHandle::Invalid;
	GPU::TextureDesc desc;
};

struct GPUBuffer {
	GPU::BufferHandle handle = GPU::BufferHandle::Invalid;
	GPU::BufferDesc desc;
};

struct ComputePipeline {
	GPU::PipelineHandle handle;
	GPU::ComputePipelineDesc desc;
	GPU::PipelineInputLayout layout;
};

struct GraphicsPipeline {
	GPU::PipelineHandle handle;
	GPU::GraphicsPipelineDesc desc;
	GPU::PipelineInputLayout layout;
};

struct RenderPass {
	GPU::PipelineHandle handle;
	GPU::RenderPassDesc desc;
};

struct PipelineResourceList {
	GPU::PipelineHandle handle;
	std::uint32_t num_resources;
};

}