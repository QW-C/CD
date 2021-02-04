Texture2D<float4> input : register(t0, space0);
RWTexture2D<float4> output : register(u0, space0);

float4 tonemapping_reinhardt(float4 color) {
	return color / (color + 1.f);
}

[numthreads(16, 16, 1)]
void main(uint3 dt_id : SV_DispatchThreadID) {
	output[dt_id.xy] = tonemapping_reinhardt(input.Load(uint3(dt_id.xy, 0)));
}