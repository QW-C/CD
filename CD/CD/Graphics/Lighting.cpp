#pragma once

#include <CD/Graphics/Lighting.hpp>
#include <CD/Graphics/Scene.hpp>
#include <CD/Graphics/Material.hpp>

namespace CD {

Lighting::Lighting(Frame& frame, MaterialSystem& material_system) :
	frame(frame),
	materials(material_system) {
	GPU::ShaderCompiler& compiler = frame.get_shader_compiler();

	GPU::ComputePipelineDesc desc {};

	GPU::CompileShaderDesc shader_desc {
		L"Resources/Shaders/Lighting.hlsl",
		L"main",
		GPU::ShaderStage::Compute
	};
	auto shader = compiler.compile_shader(shader_desc);
	desc.compute_shader = {shader->GetBufferPointer(), shader->GetBufferSize()};

	GPU::PipelineInputLayout layout {};
	set_default_samplers(layout);

	layout.num_entries = LightingInputSlot_Count;
	layout.entries[LightingInputSlot_MaterialList] = GPU::pipeline_input_list_defaults(GPU::resource_list_defaults(MaterialSystem::texture_list_size, GPU::DescriptorType::SRV, 0, 0));
	layout.entries[LightingInputSlot_LightList] = GPU::pipeline_input_buffer_defaults(GPU::DescriptorType::SRV, 0, 1);
	layout.entries[LightingInputSlot_Parameters] = GPU::pipeline_input_buffer_defaults(GPU::DescriptorType::CBV, 0, 0);
	layout.entries[LightingInputSlot_InputTextures] = GPU::pipeline_input_list_defaults(GPU::resource_list_defaults(5, GPU::DescriptorType::SRV, 0, 2));
	layout.entries[LightingInputSlot_OutputTexture] = GPU::pipeline_input_list_defaults(GPU::resource_list_defaults(1, GPU::DescriptorType::UAV, 0, 0));

	pipeline = frame.create_pipeline(desc, layout);
}

Lighting::~Lighting() = default;

void Lighting::apply(GPU::CommandBuffer& command_buffer, const Scene& scene, GPU::PipelineHandle input, GPU::PipelineHandle output) {
	const BufferAllocation& lights = scene.get_light_buffer();
	const BufferAllocation& parameters = scene.get_lighting_parameters();

	GPU::PipelineInputState input_state {};
	input_state.num_elements = LightingInputSlot_Count;

	input_state.types[LightingInputSlot_MaterialList] = GPU::PipelineInputGroupType::ResourceList;
	input_state.input_elements[LightingInputSlot_MaterialList].resource_list = materials.get_resource_list();

	input_state.types[LightingInputSlot_LightList] = GPU::PipelineInputGroupType::Buffer;
	input_state.input_elements[LightingInputSlot_LightList].buffer.buffer = lights.handle;
	input_state.input_elements[LightingInputSlot_LightList].buffer.offset = lights.offset;
	input_state.input_elements[LightingInputSlot_LightList].buffer.type = GPU::DescriptorType::SRV;

	input_state.types[LightingInputSlot_Parameters] = GPU::PipelineInputGroupType::Buffer;
	input_state.input_elements[LightingInputSlot_Parameters].buffer.buffer = parameters.handle;
	input_state.input_elements[LightingInputSlot_Parameters].buffer.offset = parameters.offset;
	input_state.input_elements[LightingInputSlot_Parameters].buffer.type = GPU::DescriptorType::CBV;

	input_state.types[LightingInputSlot_InputTextures] = GPU::PipelineInputGroupType::ResourceList;
	input_state.input_elements[LightingInputSlot_InputTextures].resource_list = input;

	input_state.types[LightingInputSlot_OutputTexture] = GPU::PipelineInputGroupType::ResourceList;
	input_state.input_elements[LightingInputSlot_OutputTexture].resource_list = output;

	const GPU::Viewport& viewport = frame.get_viewport();

	GPU::DispatchDesc dispatch;
	dispatch.compute_pipeline = pipeline->handle;
	dispatch.pipeline_input_state = input_state;
	dispatch.x = std::uint32_t((viewport.width + 7) / 8);
	dispatch.y = std::uint32_t((viewport.height + 7) / 8);
	dispatch.z = 1;

	command_buffer.add_command(dispatch);
}

Tonemapper::Tonemapper(Frame& frame) :
	frame(frame) {
	GPU::PipelineInputLayout layout {};
	layout.num_entries = 2;

	layout.entries[0] = GPU::pipeline_input_list_defaults(GPU::resource_list_defaults(1, GPU::DescriptorType::SRV, 0, 0));
	layout.entries[0].type = GPU::PipelineInputGroupType::ResourceList;

	layout.entries[1] = GPU::pipeline_input_list_defaults(GPU::resource_list_defaults(1, GPU::DescriptorType::UAV, 0, 0));
	layout.entries[1].type = GPU::PipelineInputGroupType::ResourceList;

	GPU::ComputePipelineDesc tonemapping_pipeline_desc;
	GPU::CompileShaderDesc cs_desc {
		L"Resources/Shaders/Tonemapping.hlsl",
		L"main",
		GPU::ShaderStage::Compute
	};
	auto cs = frame.get_shader_compiler().compile_shader(cs_desc);

	tonemapping_pipeline_desc.compute_shader = {cs->GetBufferPointer(), cs->GetBufferSize()};

	tonemapping_pipeline = frame.create_pipeline(tonemapping_pipeline_desc, layout);
}

void Tonemapper::apply(GPU::CommandBuffer& cb, GPU::PipelineHandle in, GPU::PipelineHandle out) {
	GPU::PipelineInputState state {};

	state.num_elements = 2;

	state.types[0] = GPU::PipelineInputGroupType::ResourceList;
	state.input_elements[0].resource_list = in;

	state.types[1] = GPU::PipelineInputGroupType::ResourceList;
	state.input_elements[1].resource_list = out;

	const GPU::Viewport& viewport = frame.get_viewport();

	GPU::DispatchDesc dispatch;
	dispatch.x = std::uint32_t((viewport.width + 15) / 16);
	dispatch.y = std::uint32_t((viewport.height + 15) / 16);
	dispatch.z = 1;
	dispatch.compute_pipeline = tonemapping_pipeline->handle;
	dispatch.pipeline_input_state = state;

	cb.add_command(dispatch);
}

}