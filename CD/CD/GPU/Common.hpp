#pragma once

#include <CD/Common/Common.hpp>

namespace CD::GPU {

constexpr std::size_t swapchain_backbuffer_count = 3;
constexpr std::size_t max_render_targets = 8;
constexpr std::size_t max_input_elements = 8;
constexpr std::size_t max_vertex_buffers = 8;
constexpr std::size_t max_timestamp_queries = 100;
constexpr std::size_t max_resource_list_ranges = 3;
constexpr std::size_t max_pipeline_layout_entries = 8;
constexpr std::size_t max_pipeline_layout_samplers = 8;
constexpr std::size_t max_resource_barriers = 8;

enum class BufferHandle : std::uint16_t {
	Null,
	Invalid = 65535
};

enum class TextureHandle : std::uint16_t {
	Null,
	Invalid = 65535
};

enum class PipelineResourceType : std::uint8_t {
	PipelineInputList,
	ComputePipeline,
	GraphicsPipeline,
	RenderPass
};

struct PipelineHandle {
	std::uint32_t handle;
	PipelineResourceType type;
};

enum CommandQueueType : std::uint8_t {
	CommandQueueType_Direct,
	CommandQueueType_Compute,
	CommandQueueType_Copy,
	CommandQueueType_Count
};

struct Signal {
	CommandQueueType queue;
	std::uint64_t value;
};

enum class BufferFormat : std::uint8_t {
	Undefined,
	R32G32B32A32_Typeless,
	R32G32B32A32_FLOAT,
	R32G32B32A32_UINT,
	R32G32B32A32_SINT,
	R32G32B32_Typeless,
	R32G32B32_FLOAT,
	R32G32B32_UINT,
	R32G32B32_SINT,
	R16G16B16A16_Typeless,
	R16G16B16A16_FLOAT,
	R16G16B16A16_UNORM,
	R16G16B16A16_UINT,
	R16G16B16A16_SNORM,
	R16G16B16A16_SINT,
	R32G32_Typeless,
	R32G32_FLOAT,
	R32G32_UINT,
	R32G32_SINT,
	R32G8X24_Typeless,
	D32_FLOAT_S8X24_UINT,
	R32_FLOAT_X8X24_Typeless,
	X32_Typeless_G8X24_UINT,
	R10G10B10A2_Typeless,
	R10G10B10A2_UNORM,
	R10G10B10A2_UINT,
	R11G11B10_FLOAT,
	R8G8B8A8_Typeless,
	R8G8B8A8_UNORM,
	R8G8B8A8_UNORM_SRGB,
	R8G8B8A8_UINT,
	R8G8B8A8_SNORM,
	R8G8B8A8_SINT,
	R16G16_Typeless,
	R16G16_FLOAT,
	R16G16_UNORM,
	R16G16_UINT,
	R16G16_SNORM,
	R16G16_SINT,
	R32_Typeless,
	D32_FLOAT,
	R32_FLOAT,
	R32_UINT,
	R32_SINT,
	R24G8_Typeless,
	D24_UNORM_S8_UINT,
	R24_UNORM_X8_Typeless,
	X24_Typeless_G8_UINT,
	R8G8_Typeless,
	R8G8_UNORM,
	R8G8_UINT,
	R8G8_SNORM,
	R8G8_SINT,
	R16_Typeless,
	R16_FLOAT,
	D16_UNORM,
	R16_UNORM,
	R16_UINT,
	R16_SNORM,
	R16_SINT,
	R8_Typeless,
	R8_UNORM,
	R8_UINT,
	R8_SNORM,
	R8_SINT,
	A8_UNORM,
	R1_UNORM,
	R8G8_B8G8_UNORM,
	G8R8_G8B8_UNORM,
	BC1_Typeless,
	BC1_UNORM,
	BC1_UNORM_SRGB,
	BC2_Typeless,
	BC2_UNORM,
	BC2_UNORM_SRGB,
	BC3_Typeless,
	BC3_UNORM,
	BC3_UNORM_SRGB,
	BC4_Typeless,
	BC4_UNORM,
	BC4_SNORM,
	BC5_Typeless,
	BC5_UNORM,
	BC5_SNORM,
	B5G6R5_UNORM,
	B5G5R5A1_UNORM,
	B8G8R8A8_UNORM,
	B8G8R8X8_UNORM,
	B8G8R8A8_Typeless,
	B8G8R8A8_UNORM_SRGB,
	B8G8R8X8_Typeless,
	B8G8R8X8_UNORM_SRGB,
	BC6H_Typeless,
	BC6H_UF16,
	BC6H_SF16,
	BC7_Typeless,
	BC7_UNORM,
	BC7_UNORM_SRGB,
	B4G4R4A4_UNORM,
	Unused
};

enum class BufferStorage {
	Device,
	Upload,
	Readback
};

enum BindFlags : std::uint16_t {
	BindFlags_None = 0x0,
	BindFlags_VertexBuffer = 0x1,
	BindFlags_IndexBuffer = 0x2,
	BindFlags_IndirectArguments = 0x4,
	BindFlags_ShaderResource = 0x8,
	BindFlags_RenderTarget = 0x10,
	BindFlags_DepthStencilTarget = 0x20,
	BindFlags_RW = 0x40
};

struct BufferDesc {
	std::uint64_t size;
	BufferStorage storage;
	BindFlags flags;
};

struct BufferView {
	BufferHandle buffer;
	BufferFormat format;
	std::uint32_t offset;
	std::uint32_t size;
	std::uint32_t stride;
};

enum class TextureDimension : std::uint8_t {
	Texture1D,
	Texture2D,
	Texture3D
};

enum class TextureViewDimension : std::uint8_t {
	Texture1D,
	Texture2D,
	Texture2DArray,
	Texture2DMS,
	TextureCube,
	Texture3D
};

struct TextureDesc {
	std::uint16_t width;
	std::uint16_t height;
	std::uint16_t depth;
	std::uint16_t mip_levels;
	std::uint16_t sample_count;
	BufferFormat format;
	TextureDimension dimension;
	BindFlags flags;
};

struct TextureView {
	TextureHandle texture;
	BufferFormat format;
	TextureViewDimension dimension;
	std::uint16_t mip_count;
	std::uint16_t depth;
	std::uint16_t mip_level;
	std::uint16_t index;
	std::uint16_t plane;
};

enum class RenderPassBeginOp : std::uint8_t {
	Discard,
	Clear,
	Preserve
};

enum class RenderPassEndOp : std::uint8_t {
	Discard,
	Resolve,
	Preserve
};

struct RenderPassRenderTargetDesc {
	RenderPassBeginOp begin_op;
	RenderPassEndOp end_op;
	BufferFormat clear_format;
	float clear_value[4];
};

struct RenderPassDepthStencilTargetDesc {
	RenderPassBeginOp depth_begin_op;
	RenderPassBeginOp stencil_begin_op;
	RenderPassEndOp depth_end_op;
	RenderPassEndOp stencil_end_op;
	BufferFormat depth_clear_format;
	BufferFormat stencil_clear_format;
	float begin_depth_clear_value;
	float end_depth_clear_value;
	std::uint8_t begin_stencil_clear_value;
	std::uint8_t end_stencil_clear_value;
};

struct RenderPassDesc {
	RenderPassRenderTargetDesc render_targets[max_render_targets];
	RenderPassDepthStencilTargetDesc* depth_stencil_target;
	std::uint32_t num_render_targets;
};

enum class DescriptorType {
	SRV,
	UAV,
	CBV
};

enum class PipelineInputGroupType {
	ResourceList,
	Buffer
};

struct ResourceListDesc {
	DescriptorType type;
	std::uint32_t binding_slot;
	std::uint32_t binding_space;
	std::uint32_t num_resources;
	std::uint32_t offset;
};

struct PipelineInputListDesc {
	ResourceListDesc resource_lists[max_resource_list_ranges];
	std::uint32_t num_resource_lists;
};

struct PipelineInputBufferDesc {
	DescriptorType type;
	std::uint32_t binding_slot;
	std::uint32_t binding_space;
};

struct PipelineInputGroup {
	PipelineInputGroupType type;
	union {
		PipelineInputListDesc resource_lists;
		PipelineInputBufferDesc buffer;
	};
};

enum class Filter : std::uint8_t {
	Point,
	Linear
};

enum class AddressMode : std::uint8_t {
	Wrap,
	Mirror,
	Clamp,
	MirrorClamp
};

enum class CompareOp : std::uint8_t {
	Never,
	Less,
	Equal,
	LessEqual,
	Greater,
	NotEqual,
	GreaterEqual,
	Always
};

struct SamplerDesc {
	bool anisotropy_enable;
	std::uint8_t max_anisotropy;
	Filter min_filter;
	Filter mag_filter;
	Filter mip_filter;
	AddressMode address_u;
	AddressMode address_v;
	AddressMode address_w;
	float mip_lod_bias;
	CompareOp comparison_func;
	float min_lod;
	float max_lod;
};

struct PipelineInputLayout {
	PipelineInputGroup entries[max_pipeline_layout_entries];
	SamplerDesc samplers[max_pipeline_layout_samplers];
	std::uint16_t sampler_binding_slots[max_pipeline_layout_samplers];
	std::uint16_t sampler_binding_spaces[max_pipeline_layout_samplers];
	std::uint16_t num_entries;
	std::uint16_t num_samplers;
};

struct Shader {
	const void* bytecode;
	std::size_t size;
};

struct InputElement {
	const char* name;
	std::uint32_t index;
	BufferFormat format;
	std::uint16_t slot;
	std::uint16_t offset;
	std::uint16_t instance_data_rate;
	bool instance_element;
};

struct VertexInputDesc {
	const InputElement* elements;
	std::uint32_t num_elements;
};

enum class PrimitiveTopology : std::uint8_t {
	PointList,
	LineList,
	LineStrip,
	TriangleList,
	TriangleStrip,
	LineListAdjacency,
	LineStripAdjacency,
	TriangleListAdjacency,
	TriangleStripAdjacency,
	PatchList
};

enum class FillMode : std::uint8_t {
	Wireframe,
	Solid
};

enum class CullMode : std::uint8_t {
	None,
	Back,
	Front
};

enum class FrontFace : std::uint8_t {
	Clockwise,
	CounterClockwise
};

struct RasterizerParameters {
	FillMode fill_mode;
	CullMode cull_mode;
	FrontFace front_face;
	bool depth_clip_enable;
	bool multisample_enable;
	bool antialiased_line_enable;
	bool conservative_raster_enable;
	std::int32_t depth_bias;
	float depth_bias_clamp;
	float depth_bias_slope;
	std::uint32_t forced_sample_count;
};

enum class BlendFactor : std::uint8_t {
	Zero,
	One,
	SrcColor,
	InvSrcColor,
	SrcAlpha,
	InvSrvAlpha,
	DstAlpha,
	InvDstAlpha,
	DstColor,
	InvDstColor,
	SrcAlphaSat,
	BlendFactor,
	InvBlendFactor,
	Src1Color,
	InvSrc1Color,
	Src1Alpha,
	InvSrc1Alpha
};

enum class BlendOp : std::uint8_t {
	Add,
	Substract,
	ReverseSubstract,
	Min,
	Max
};

enum class LogicOp : std::uint8_t {
	Clear,
	Set,
	Copy,
	CopyInv,
	NoOp,
	Invert,
	AND,
	NAND,
	OR,
	NOR,
	XOR,
	Equivalent,
	ANDReverse,
	InvAND,
	ORReverse,
	InvOR
};

struct RenderTargetBlend {
	bool blend_enable;
	bool logic_op_enable;
	BlendFactor src_blend;
	BlendFactor dst_blend;
	BlendOp blend_op;
	BlendFactor src_alpha_blend;
	BlendFactor dst_alpha_blend;
	BlendOp alpha_blend_op;
	LogicOp logic_op;
	std::uint8_t render_target_write_mask;
};

struct BlendDesc {
	RenderTargetBlend blend[max_render_targets];
};

enum class StencilOp : std::uint8_t {
	Keep,
	Zero,
	Replace,
	IncrementSaturate,
	DecrementSaturate,
	Invert,
	IncrementWrap,
	DecrementWrap
};

enum class DepthWriteMask : std::uint8_t {
	Zero,
	All
};

struct DepthStencilOp {
	StencilOp stencil_fail_op;
	StencilOp stencil_depth_fail_op;
	StencilOp stencil_pass_op;
	CompareOp stencil_op;
};

struct DepthStencilTargetDesc {
	bool depth_enable;
	DepthWriteMask depth_write_mask;
	CompareOp depth_compare_op;
	bool stencil_enable;
	std::uint8_t stencil_read_mask;
	std::uint8_t stencil_write_mask;
	DepthStencilOp front_face_op;
	DepthStencilOp back_face_op;
};

struct GraphicsPipelineDesc {
	VertexInputDesc input_layout;
	Shader vs;
	Shader ps;
	Shader gs;
	Shader ds;
	Shader hs;
	RasterizerParameters rasterizer;
	BlendDesc blend_desc;
	DepthStencilTargetDesc depth_stencil;
	BufferFormat depth_stencil_format;
	PrimitiveTopology primitive_topology;
	BufferFormat render_target_format[max_render_targets];
	std::uint16_t render_target_count;
	std::uint16_t sample_count;
	std::uint32_t sample_mask;
	bool alpha_to_coverage_enable;
	bool independent_blend_enable;
};

struct ComputePipelineDesc {
	Shader compute_shader;
};

struct PipelineInputBuffer {
	BufferHandle buffer;
	std::uint32_t offset;
	DescriptorType type;
};

struct PipelineInputState {
	PipelineInputGroupType types[max_pipeline_layout_entries];
	union {
		PipelineHandle resource_list;
		PipelineInputBuffer buffer;
	} input_elements[max_pipeline_layout_entries];
	std::uint32_t num_elements;
};

struct Viewport {
	float top_left_x;
	float top_left_y;
	float width;
	float height;
	float min_z;
	float max_z;
};

struct Scissor {
	std::uint32_t left;
	std::uint32_t top;
	std::uint32_t right;
	std::uint32_t bottom;
};

enum class ResourceState {
	Common,
	RTV,
	UAV,
	DepthWrite,
	DepthRead
};

struct SwapChainDesc {
	void* handle;
	std::uint32_t width;
	std::uint32_t height;
};

struct DeviceFeatureInfo {
	bool uma;
	std::uint64_t timestamp_frequency[CommandQueueType_Count];
};

}