struct DepthPassConstants {
	float4x4 view_projection;
};

struct MeshInstanceConstants {
	uint transform_index;
	uint _;
};

StructuredBuffer<float4x4> transforms : register(t0, space0);

ConstantBuffer<DepthPassConstants> depth_constants : register(b0, space0);

ConstantBuffer<MeshInstanceConstants> mesh_constants : register(b1, space0);

struct VSIn {
	float3 position : POSITION;
};

struct VSOut {
	float4 position : SV_Position;
};

VSOut vs_main(VSIn input) {
	const float4x4 transform = transforms[mesh_constants.transform_index];
	const float4x4 local_to_clip = mul(transform, depth_constants.view_projection);

	VSOut output;

	output.position = mul(float4(input.position, 1.f), local_to_clip);

	return output;
}