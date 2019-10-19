struct VSOutput 
{
	float4 Position : SV_POSITION;
	float2 UV		: TEXCOORD0;
};

SamplerState	uSampler0		: register(s0);
Texture2D		Texture			: register(t0);

float4 main(VSOutput input) : SV_TARGET
{
	float4 texel = Texture.Sample(uSampler0, input.UV);
	// luma trick to mimic HDR, and take advantage of 16 bit buffers shader toy provides.
	float lum = dot(texel.rgb, float3(0.2126,0.7152,0.0722)) * 1.8;
	texel = texel * (1.0 + 0.2*lum*lum*lum);
    return texel * texel;
}