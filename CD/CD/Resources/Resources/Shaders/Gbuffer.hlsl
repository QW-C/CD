//reference for deferred texturing :
//https://mynameismjp.wordpress.com/2016/03/25/bindless-texturing-for-deferred-rendering-and-decals/

#include "Resources/Shaders/Utils.hlsli"

struct GBufferConstants {
	float4x4 view_projection;
};

struct MeshInstanceConstants {
	uint transform_index;
	uint material_index;
};

StructuredBuffer<float4x4> transforms : register(t0, space0);

ConstantBuffer<GBufferConstants> pass_constants : register(b0, space0);

ConstantBuffer<MeshInstanceConstants> instance_constants : register(b1, space0);

struct VSIn {
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : UV;
};

struct VSOut {
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : UV;
};

struct GBuffer {
	uint4 normal : SV_Target0;
	float2 uv : SV_Target1;
	float4 duv : SV_Target2;
	uint material : SV_Target3;
};

VSOut vs_main(VSIn input) {
	const float4x4 transform = transforms[instance_constants.transform_index];
	const float4x4 local_to_clip = mul(transform, pass_constants.view_projection);

	VSOut output;
	
	output.position = mul(float4(input.position, 1.f), local_to_clip);

	output.normal = normalize(mul(float4(input.normal, 0.f), transform)).xyz;
	output.tangent = normalize(mul(float4(input.tangent, 0.f), transform)).xyz;

	output.uv = input.uv;
	
	return output;
}

GBuffer ps_main(VSOut input) {
	GBuffer output;

	output.normal = uint4(normal_tangent_16(normalize(input.normal), normalize(input.tangent).xyz), 0.f);
	output.uv = input.uv;
	output.duv = float4(ddx(input.uv), ddy(input.uv));
	output.material = instance_constants.material_index;
	
	return output;
}