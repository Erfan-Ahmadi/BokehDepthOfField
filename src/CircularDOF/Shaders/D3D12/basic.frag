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

SamplerState	uSampler0		: register(s0);

// material parameters
Texture2D textureMaps[]			: register(t3);

float4 main(PsIn input) : SV_TARGET
{
	//load albedo
	float3 albedo = textureMaps[albedoMap].Sample(uSampler0, input.uv).rgb;
	return float4(albedo, 1);
}