#include "Resources/Shaders/Vertex.hlsli"
#include "Resources/Shaders/Samplers.hlsli"

struct SkyCameraConstants {
	float4x4 camera_transform;
	float4x4 inv_projection;
};

ConstantBuffer<SkyCameraConstants> vertex_constants : register(b0, space0);
TextureCube environment : register(t0, space0);

struct VSOut {
	float4 position : SV_Position;
	float3 direction : DIRECTION;
};

VSOut vs_main(uint vertex_id : SV_VertexID) {
	float4x4 view = vertex_constants.camera_transform;
	view._41 = 0.f;
	view._42 = 0.f;
	view._43 = 0.f;
	
	const float4 vertex = float4(g_triangle_pos[vertex_id], 1.f, 1.f);
	
	VSOut output;

	output.position = vertex;

	output.direction = mul(vertex, vertex_constants.inv_projection).xyz;
	output.direction = mul(float4(output.direction, 1.f), view).xyz;

	return output;
}

float4 ps_main(VSOut vs_data) : SV_Target0 {
	return environment.Sample(sampler_linear_wrap, vs_data.direction);
}