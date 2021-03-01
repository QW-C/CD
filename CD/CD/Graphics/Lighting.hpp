#pragma once

#include <CD/Graphics/Frame.hpp>
#include <CD/Common/Debug.hpp>
#include <vector>

namespace CD {

class Scene;
class MaterialSystem;

class Lighting {
public:
	Lighting(Frame&, MaterialSystem&);
	~Lighting();

	void apply(GPU::CommandBuffer&, const Scene&, GPU::PipelineHandle input, GPU::PipelineHandle output);
private:
	Frame& frame;
	MaterialSystem& materials;

	const ComputePipeline* pipeline;

	enum LightingInputSlot : std::uint8_t {
		LightingInputSlot_MaterialList,
		LightingInputSlot_LightList,
		LightingInputSlot_Parameters,
		LightingInputSlot_InputTextures,
		LightingInputSlot_OutputTexture,
		LightingInputSlot_Count
	};
};

class Tonemapper {
public:
	Tonemapper(Frame&);

	void apply(GPU::CommandBuffer&, GPU::PipelineHandle in, GPU::PipelineHandle out);
private:
	Frame& frame;

	const ComputePipeline* tonemapping_pipeline;
};

}