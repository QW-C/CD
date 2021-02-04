uint3 normal_tangent_16(float3 u, float3 v) {
	const uint3 u16 = f32tof16(u);
	const uint3 v16 = f32tof16(v);
	return (u16 << 16) | v16;
}

void get_normal_tangent_16(uint3 u, out float3 v, out float3 w) {
	const uint3 f = asuint(f16tof32(u));
	v = f16tof32(u >> 16);
	w = f16tof32(u);
}