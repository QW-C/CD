#include "Resources/Shaders/Utils.hlsli"
#include "Resources/Shaders/Common.hlsli"
#include "Resources/Shaders/Samplers.hlsli"

struct LightingConstants {
	float4x4 inv_view_projection;
	float4x4 camera_transform;
	float2 inv_screen_dimensions;
	uint num_lights;
};

Texture2D<float4> materials[] : register(t0, space0);
RWTexture2D<float4> output_texture : register(u0, space0);

ConstantBuffer<LightingConstants> parameters : register(b0, space0);

StructuredBuffer<Light> lights : register(t0, space1);

Texture2D<uint4> normals : register(t0, space2);
Texture2D<float2> uv : register(t1, space2);
Texture2D<float4> duv : register(t2, space2);
Texture2D<uint> material_indices : register(t3, space2);
Texture2D<float> depth_buffer : register(t4, space2);

enum TextureIndex {
	Albedo,
	Normal,
	Metallicity,
	Roughness
};

[numthreads(8, 8, 1)]
void main(uint3 dt_id : SV_DispatchThreadID) {
	const uint material_idx = material_indices[dt_id.xy];

	if(material_idx) {
		const float4 pixel_duv = duv.Load(uint3(dt_id.xy, 0));
		const float2 pixel_uv = uv.Load(uint3(dt_id.xy, 0));

		const uint3 nt = normals.Load(uint3(dt_id.xy, 0)).xyz;
		float3 n = 0.f;
		float3 t = 0.f;
		get_normal_tangent_16(nt, n, t);
		const float3x3 tbn = float3x3(t, cross(n, t), n);

		//https://mynameismjp.wordpress.com/2009/03/10/reconstructing-position-from-depth
		const float screen_depth = depth_buffer.Load(uint3(dt_id.xy, 0));
		const float2 p = (dt_id.xy + .5f) * parameters.inv_screen_dimensions;
		const float2 screen_uv = float2(p.x * 2.f - 1.f, (1.f - p.y) * 2.f - 1.f);
		float4 clip = float4(screen_uv.xy, screen_depth, 1.f);
		clip = mul(clip, parameters.inv_view_projection);
		const float3 position = clip.xyz / clip.w;

		LightingData lighting;

		lighting.position = position;
		lighting.camera = parameters.camera_transform._41_42_43;

		lighting.albedo = materials[NonUniformResourceIndex(material_idx + TextureIndex::Albedo)].SampleGrad(sampler_anisotropy16, pixel_uv, pixel_duv.xy, pixel_duv.zw).xyz;

		float3 surface_normal = 0.f;
		surface_normal.xyz = materials[NonUniformResourceIndex(material_idx + TextureIndex::Normal)].SampleGrad(sampler_anisotropy16, pixel_uv, pixel_duv.xy, pixel_duv.zw).xyz * 2.f - 1.f;
		lighting.normal = normalize(mul(surface_normal, tbn));

		lighting.metallicity = materials[NonUniformResourceIndex(material_idx + TextureIndex::Metallicity)].SampleGrad(sampler_anisotropy16, pixel_uv, pixel_duv.xy, pixel_duv.zw).x;
		lighting.roughness = materials[NonUniformResourceIndex(material_idx + TextureIndex::Roughness)].SampleGrad(sampler_anisotropy16, pixel_uv, pixel_duv.xy, pixel_duv.zw).x;

		lighting.lights = lights;
		lighting.num_lights = parameters.num_lights;

		output_texture[dt_id.xy] = compute_lighting(lighting);
	}
	else {
		output_texture[dt_id.xy] = float4(1.f, 0.f, 1.f, 1.f);
	}
}