static const float PI = 3.14159265f;

struct Light {
	float4 position;
	float4 color;
	float intensity;
	float unused[3];
};

struct LightingData {
	float3 position;
	float3 camera;
	float3 albedo;
	float3 normal;
	float metallicity;
	float roughness;
	StructuredBuffer<Light> lights;
	uint num_lights;
};

float d_ggx(float a, float nh) {
	const float r = a * a;
	float d = (nh * r - nh) * nh + 1.f; //* (r - 1.f) + 1.f;
	d *= d;
	return r * (1.f / PI) / d;
}

//https://google.github.io/filament/Filament.html
float g_smithggx(float a, float nv, float nl) {
	const float g1_v = nl * sqrt(nv * nv * (1.f - a) + a);
	const float g1_l = nv * sqrt(nl * nl * (1.f - a) + a);
	return .5f / (g1_v + g1_l);
}

float3 fresnel_schlick(float3 f0, float vh) {
	return f0 + (1.f - f0) * pow(1.f - vh, 5.f);
}

float3 eval_light(float intensity, float3 albedo, float metallicity, float roughness,
	float nv, float nh, float nl, float vh) {
	const float3 f0 = lerp(.04f, albedo, metallicity);
	const float3 f = fresnel_schlick(f0, vh);

	const float3 kd = (1.f - f) * (1.f - metallicity);
	const float3 d = kd * albedo * (1.f / PI);
	
	const float a = roughness * roughness;
	const float3 s = d_ggx(a, nh) * f * g_smithggx(a, nv, nl) * .25f / (nl * nv);

	return nl * intensity * (d + s);
}

float4 compute_lighting(LightingData lighting) {
	float4 color  = 0.f;

	for(uint light_index = 0; light_index < lighting.num_lights; ++light_index) {
		const float3 l = normalize(lighting.lights[light_index].position.xyz - lighting.position);
		const float3 v = normalize(lighting.camera - lighting.position);
		const float3 h = normalize(l + lighting.camera);

		const float nl = saturate(dot(lighting.normal, l));

		const float d = length(l);
		const float intensity =  lighting.lights[light_index].intensity / (d * d); 

		if(nl > 0.f) {
			const float nv = saturate(dot(lighting.normal, v));
			const float nh = saturate(dot(lighting.normal, h));
			const float vh = saturate(dot(v, h));

			color += float4(eval_light(intensity, lighting.albedo, lighting.metallicity, lighting.roughness, nv, nh, nl, vh), 1.f);
		}
	}

	return color;
}