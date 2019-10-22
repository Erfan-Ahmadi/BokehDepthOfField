cbuffer cbTextureRootConstants : register(b2) 
{
	uint albedoMap;
}

struct PsIn
{    
    float3 normal	: TEXCOORD0;
	float3 pos		: TEXCOORD1;
	float2 uv		: TEXCOORD2;
    float4 position : SV_Position;
};

SamplerState	samplerLinear		: register(s0);

// material parameters
Texture2D textureMaps[]			: register(t3);

float4 main(PsIn input) : SV_TARGET
{
	//load albedo
	float3 albedo = textureMaps[albedoMap].Sample(samplerLinear, input.uv).rgb;
	// luma trick to mimic HDR, and take advantage of 16 bit buffers shader toy provides.
	float lum = dot(albedo.rgb, float3(0.2126,0.7152,0.0722)) * 1.8;
	albedo = albedo * (1.0 + 0.2*lum*lum*lum);
	return float4(albedo * albedo, 1);
}